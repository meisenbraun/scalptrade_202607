#!/usr/bin/env python3

import argparse
import math
import socket
import threading
import time
from collections import OrderedDict
from dataclasses import dataclass, field


TRADE_MESSAGE_SIZE = 38
QUOTE_MESSAGE_SIZE = 57
ORDER_MESSAGE_SIZE = 48

MAX_SEQUENCE = 9_999_999

# Hardcoded test values.
TRADE_QTY = 100
TRADE_PRICE_PENNIES = 12_345

BID_QTY = 200
BID_PRICE_PENNIES = 12_340
ASK_QTY = 150
ASK_PRICE_PENNIES = 10


@dataclass(frozen=True)
class Order:
    symbol: str
    timestamp_ns: int
    side: str
    qty: int
    price_pennies: int
    correlation_id: int


@dataclass
class LatencySamples:
    full_round_trip_ns: list[int] = field(default_factory=list)
    server_to_order_ns: list[int] = field(default_factory=list)
    client_to_server_ns: list[int] = field(default_factory=list)

    malformed_orders: int = 0
    unmatched_orders: int = 0

    def add(
            self,
            quote_timestamp_ns: int,
            order_timestamp_ns: int,
            receive_timestamp_ns: int,
    ) -> None:
        # Server quote timestamp -> order received by server.
        self.full_round_trip_ns.append(
            receive_timestamp_ns - quote_timestamp_ns
        )

        # Server quote timestamp -> client-created order timestamp.
        self.server_to_order_ns.append(
            order_timestamp_ns - quote_timestamp_ns
        )

        # Client-created order timestamp -> order received by server.
        self.client_to_server_ns.append(
            receive_timestamp_ns - order_timestamp_ns
        )


def encode_field(value: str | int, width: int) -> bytes:
    """Encode an ASCII field and right-pad it with spaces."""
    encoded = str(value).encode("ascii")

    if len(encoded) > width:
        raise ValueError(
            f"value {value!r} does not fit in a {width}-byte field"
        )

    return encoded.ljust(width, b" ")


def decode_int_field(field_bytes: bytes, name: str) -> int:
    text = field_bytes.decode("ascii").rstrip(" ")

    if not text or not text.isdigit():
        raise ValueError(
            f"invalid {name} field: {field_bytes!r}"
        )

    return int(text)


def make_trade_message(symbol: bytes) -> bytes:
    message = (
            b"T"
            + symbol
            + encode_field(time.time_ns(), 19)
            + encode_field(TRADE_QTY, 6)
            + encode_field(TRADE_PRICE_PENNIES, 6)
    )

    assert len(message) == TRADE_MESSAGE_SIZE
    return message


def make_quote_message(
        symbol: bytes,
        sequence: int,
) -> tuple[bytes, int]:
    timestamp_ns = time.time_ns()

    message = (
            b"Q"
            + symbol
            + encode_field(timestamp_ns, 19)
            + encode_field(BID_QTY, 6)
            + encode_field(BID_PRICE_PENNIES, 6)
            + encode_field(ASK_QTY, 6)
            + encode_field(ASK_PRICE_PENNIES, 6)
            + encode_field(sequence, 7)
    )

    assert len(message) == QUOTE_MESSAGE_SIZE
    return message, timestamp_ns


def parse_order(message: bytes) -> Order:
    if len(message) != ORDER_MESSAGE_SIZE:
        raise ValueError(
            f"expected {ORDER_MESSAGE_SIZE} bytes, "
            f"got {len(message)}"
        )

    # Order layout:
    #
    #  0 -  5: symbol
    #  6 - 24: timestamp
    # 25 - 28: side
    # 29 - 34: quantity
    # 35 - 40: price
    # 41 - 47: correlation ID

    symbol = message[0:6].decode("ascii").rstrip(" ")

    timestamp_ns = decode_int_field(
        message[6:25],
        "timestamp",
    )

    side_field = message[25:29].decode("ascii")

    qty = decode_int_field(
        message[29:35],
        "quantity",
    )

    price_pennies = decode_int_field(
        message[35:41],
        "price",
    )

    correlation_id = decode_int_field(
        message[41:48],
        "correlation ID",
    )

    if side_field not in ("BUY ", "SELL"):
        raise ValueError(
            f"invalid side field: {side_field!r}"
        )

    return Order(
        symbol=symbol,
        timestamp_ns=timestamp_ns,
        side=side_field.rstrip(" "),
        qty=qty,
        price_pennies=price_pennies,
        correlation_id=correlation_id,
    )


def recv_exact(
        connection: socket.socket,
        size: int,
        stop_event: threading.Event,
) -> bytes | None:
    """
    Receive exactly one fixed-width record.

    Returns None if the peer disconnects or the server is stopping.
    """
    buffer = bytearray()

    while len(buffer) < size and not stop_event.is_set():
        try:
            chunk = connection.recv(size - len(buffer))
        except socket.timeout:
            continue

        if not chunk:
            return None

        buffer.extend(chunk)

    if len(buffer) != size:
        return None

    return bytes(buffer)


def send_market_data(
        connection: socket.socket,
        symbol: bytes,
        interval_ms: int,
        pending_quotes: OrderedDict[int, int],
        pending_quotes_lock: threading.Lock,
        stop_event: threading.Event,
        verbose: bool,
) -> None:
    sequence = 1

    try:
        while not stop_event.is_set():
            trade = make_trade_message(symbol)
            connection.sendall(trade)

            quote, quote_timestamp_ns = make_quote_message(
                symbol,
                sequence,
            )

            # Make the correlation visible before sending the quote.
            # Otherwise, an extremely fast response could reach the order
            # thread before this mapping had been inserted.
            with pending_quotes_lock:
                pending_quotes[sequence] = quote_timestamp_ns

            try:
                connection.sendall(quote)
            except OSError:
                with pending_quotes_lock:
                    pending_quotes.pop(sequence, None)

                raise

            if verbose:
                print(f"Sent trade: {trade!r}")
                print(f"Sent quote: {quote!r}")

            sequence += 1

            if sequence > MAX_SEQUENCE:
                sequence = 1

            if interval_ms > 0:
                if stop_event.wait(interval_ms / 1000.0):
                    break

    except OSError as error:
        if not stop_event.is_set():
            print(
                f"Market-data client disconnected: {error}"
            )

        stop_event.set()


def receive_orders(
        connection: socket.socket,
        expected_symbol: str,
        pending_quotes: OrderedDict[int, int],
        pending_quotes_lock: threading.Lock,
        stop_event: threading.Event,
        samples: LatencySamples,
        verbose: bool,
) -> None:
    while not stop_event.is_set():
        message = recv_exact(
            connection,
            ORDER_MESSAGE_SIZE,
            stop_event,
        )

        if message is None:
            if not stop_event.is_set():
                print("Order-entry client disconnected")

            stop_event.set()
            return

        # This is the earliest user-space observation of the complete
        # order record.
        receive_timestamp_ns = time.time_ns()

        try:
            order = parse_order(message)
        except (UnicodeDecodeError, ValueError) as error:
            samples.malformed_orders += 1
            print(f"Ignoring malformed order: {error}")
            continue

        if order.symbol != expected_symbol:
            samples.malformed_orders += 1

            print(
                "Ignoring order with unexpected symbol: "
                f"{order.symbol!r}"
            )
            continue

        with pending_quotes_lock:
            quote_timestamp_ns = pending_quotes.pop(
                order.correlation_id,
                None,
            )

        if quote_timestamp_ns is None:
            samples.unmatched_orders += 1

            if verbose:
                print(
                    "No matching quote for correlation ID "
                    f"{order.correlation_id}"
                )

            continue

        samples.add(
            quote_timestamp_ns=quote_timestamp_ns,
            order_timestamp_ns=order.timestamp_ns,
            receive_timestamp_ns=receive_timestamp_ns,
        )

        if verbose:
            print(f"Received order: {message!r}")


def nearest_rank(
        sorted_values: list[int],
        percentile: int,
) -> int:
    index = max(
        0,
        math.ceil(
            (percentile / 100.0) * len(sorted_values)
        ) - 1,
        )

    return sorted_values[index]


def print_latency_line(
        name: str,
        values: list[int],
) -> None:
    if not values:
        print(f"{name:<24} count=0")
        return

    ordered = sorted(values)

    print(
        f"{name:<24} "
        f"count={len(ordered):,}  "
        f"min={ordered[0]:,} ns  "
        f"p50={nearest_rank(ordered, 50):,} ns  "
        f"p90={nearest_rank(ordered, 90):,} ns  "
        f"p95={nearest_rank(ordered, 95):,} ns  "
        f"max={ordered[-1]:,} ns"
    )


def print_statistics(samples: LatencySamples) -> None:
    print("\nLatency statistics (nearest-rank percentiles)")

    print_latency_line(
        "Full round trip",
        samples.full_round_trip_ns,
    )

    print_latency_line(
        "Server to order",
        samples.server_to_order_ns,
    )

    print_latency_line(
        "Client to server",
        samples.client_to_server_ns,
    )

    print(
        f"Malformed orders: {samples.malformed_orders:,}"
    )

    print(
        f"Unmatched orders: {samples.unmatched_orders:,}"
    )


def close_connection(
        connection: socket.socket | None,
) -> None:
    if connection is None:
        return

    try:
        connection.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass

    connection.close()


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Fixed-width market-data and order-entry "
            "latency test server."
        )
    )

    parser.add_argument(
        "symbol",
        help="ASCII symbol, at most 6 bytes",
    )

    parser.add_argument(
        "--host",
        default="0.0.0.0",
        help="address to bind; default: %(default)s",
    )

    parser.add_argument(
        "--port",
        type=int,
        default=9000,
        help=(
            "market-data TCP port; "
            "default: %(default)s"
        ),
    )

    parser.add_argument(
        "--order-port",
        type=int,
        default=9001,
        help=(
            "order-entry TCP port; "
            "default: %(default)s"
        ),
    )

    parser.add_argument(
        "--interval-ms",
        type=int,
        default=1000,
        help=(
            "milliseconds between trade/quote pairs; "
            "0 sends continuously; default: %(default)s"
        ),
    )

    parser.add_argument(
        "--verbose",
        action="store_true",
        help=(
            "print every message; this significantly "
            "distorts latency"
        ),
    )

    args = parser.parse_args()

    if args.interval_ms < 0:
        parser.error("--interval-ms cannot be negative")

    if args.port == args.order_port:
        parser.error(
            "--port and --order-port must be different"
        )

    try:
        symbol = encode_field(args.symbol, 6)
    except (UnicodeEncodeError, ValueError) as error:
        parser.error(str(error))

    stop_event = threading.Event()

    pending_quotes: OrderedDict[int, int] = (
        OrderedDict()
    )

    pending_quotes_lock = threading.Lock()
    samples = LatencySamples()

    market_data_client: socket.socket | None = None
    order_entry_client: socket.socket | None = None
    sender_thread: threading.Thread | None = None

    with (
        socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM,
        ) as market_data_server,
        socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM,
        ) as order_entry_server,
    ):
        for server in (
                market_data_server,
                order_entry_server,
        ):
            server.setsockopt(
                socket.SOL_SOCKET,
                socket.SO_REUSEADDR,
                1,
            )

        market_data_server.bind(
            (args.host, args.port)
        )

        order_entry_server.bind(
            (args.host, args.order_port)
        )

        market_data_server.listen(1)
        order_entry_server.listen(1)

        print(
            "Market data listening on "
            f"{args.host}:{args.port}"
        )

        print(
            "Order entry listening on "
            f"{args.host}:{args.order_port}"
        )

        print(f"Symbol field: {symbol!r}")
        print(f"Interval: {args.interval_ms} ms")

        try:
            market_data_client, market_data_address = (
                market_data_server.accept()
            )

            market_data_client.setsockopt(
                socket.IPPROTO_TCP,
                socket.TCP_NODELAY,
                1,
            )

            print(
                "Market-data client connected: "
                f"{market_data_address}"
            )

            order_entry_client, order_entry_address = (
                order_entry_server.accept()
            )

            order_entry_client.setsockopt(
                socket.IPPROTO_TCP,
                socket.TCP_NODELAY,
                1,
            )

            # Allows the receive loop to periodically notice that
            # the market-data thread has stopped.
            order_entry_client.settimeout(0.5)

            print(
                "Order-entry client connected: "
                f"{order_entry_address}"
            )

            sender_thread = threading.Thread(
                target=send_market_data,
                args=(
                    market_data_client,
                    symbol,
                    args.interval_ms,
                    pending_quotes,
                    pending_quotes_lock,
                    stop_event,
                    args.verbose,
                ),
                name="market-data-sender",
            )

            sender_thread.start()

            receive_orders(
                connection=order_entry_client,
                expected_symbol=args.symbol,
                pending_quotes=pending_quotes,
                pending_quotes_lock=pending_quotes_lock,
                stop_event=stop_event,
                samples=samples,
                verbose=args.verbose,
            )

        except KeyboardInterrupt:
            print("\nServer stopped")
            stop_event.set()

        finally:
            stop_event.set()

            close_connection(market_data_client)
            close_connection(order_entry_client)

            if sender_thread is not None:
                sender_thread.join()

            print_statistics(samples)


if __name__ == "__main__":
    main()
#!/usr/bin/env python3

import argparse
import socket
import time


TRADE_MESSAGE_SIZE = 38
QUOTE_MESSAGE_SIZE = 50

# Hardcoded test values.
TRADE_QTY = 100
TRADE_PRICE_PENNIES = 12_345

BID_QTY = 200
BID_PRICE_PENNIES = 12_340
ASK_QTY = 150
#ASK_PRICE_PENNIES = 12_350
ASK_PRICE_PENNIES = 10


def encode_field(value: str | int, width: int) -> bytes:
    """Encode an ASCII field and right-pad it with spaces."""
    encoded = str(value).encode("ascii")

    if len(encoded) > width:
        raise ValueError(
            f"value {value!r} does not fit in a {width}-byte field"
        )

    return encoded.ljust(width, b" ")


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


def make_quote_message(symbol: bytes) -> bytes:
    message = (
            b"Q"
            + symbol
            + encode_field(time.time_ns(), 19)
            + encode_field(BID_QTY, 6)
            + encode_field(BID_PRICE_PENNIES, 6)
            + encode_field(ASK_QTY, 6)
            + encode_field(ASK_PRICE_PENNIES, 6)
    )

    assert len(message) == QUOTE_MESSAGE_SIZE
    return message


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Fixed-width trade and quote test server."
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
        help="TCP port; default: %(default)s",
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

    args = parser.parse_args()

    if args.interval_ms < 0:
        parser.error("--interval-ms cannot be negative")

    try:
        symbol = encode_field(args.symbol, 6)
    except (UnicodeEncodeError, ValueError) as error:
        parser.error(str(error))

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        server.bind((args.host, args.port))
        server.listen(1)

        print(f"Listening on {args.host}:{args.port}")
        print(f"Symbol field: {symbol!r}")
        print(f"Interval: {args.interval_ms} ms")

        client, address = server.accept()

        with client:
            print(f"Client connected: {address}")

            try:
                while True:
                    trade = make_trade_message(symbol)
                    quote = make_quote_message(symbol)

                    client.sendall(trade)
                    client.sendall(quote)

                    print(f"Sent trade: {trade!r}")
                    print(f"Sent quote: {quote!r}")

                    if args.interval_ms > 0:
                        time.sleep(args.interval_ms / 1000.0)

            except (
                    BrokenPipeError,
                    ConnectionResetError,
                    ConnectionAbortedError,
            ):
                print("Client disconnected")

            except KeyboardInterrupt:
                print("\nServer stopped")


if __name__ == "__main__":
    main()
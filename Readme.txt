SCALP Trade Programming Assessment
Matthew Eisenbraun
July 16, 2026

Build Instructions
make clean
make

Overview
This project was developed and tested on Ubuntu 24.04 running on WSL 2 on
Windows 10. g++ 15.2.0 was used.

The program uses three threads to be able to create orders with low latency
from the market data event that caused the trade. All three threads set the
CPU affinity mask to lock them to individual single cores.

The main thread runs the epoll loop and networking I/O with non-blocking sockets.
Edge-triggered epoll is used more as demonstration of the technique than out of real
need for this problem. If the problem were scaled up, as in a real production system,
edge-triggered would be a more natural fit. Further, the read loop is designed to
accommodate both partial and multiple records, as TCP may deliver either.

When the epoll detects data is available data is read into a SPSCQueue lock-free
queue for consumption by the business logic thread. This design allows records to be
processed with minimal delay, without waiting for the read loop to be idle.

The business logic thread is encapsulated by the ProcessQueueThread. This thread
is non-blocking and continually drains the SPSCQueue. This thread handles data
parsing, VWAP calculation, and Order triggering (but not sending). The VWAP
Window is handled by checking the current time against the next window expiration
time. std::chrono::system_clock is used for this check, because system_clock is
designed for wall time. Then, when the thread detects that VWAP is favorable
(vwap < bid for sells, vwap > ask for buys) with respect to the latest quote it
copies the min(current quote size, user-specified maximum size) and price
(ask for buy orders, bid for sell orders) into a struct that is available for
reading by the third thread. It also sets an atomic variable to mark the data as
available to be read.

The third thread is the order entry thread (SendOrderThread). It spins over the
atomic variable mentioned above. When the flag is set the thread copies the data
from the shared structure into local memory and uses those parameters to send a new
order.

Assumptions
The market data feed will only contain a single symbol. This assumption seems warranted
from prior experience with market feeds, which generally have both high-volume dedicated
market feeds, and lower volume aggerated feeds combining multiple markets. This assumption
was also justified by the loose and permissive language in the assessment description.

Design Tradeoffs
The SPSCQueue has a fixed capacity hardcoded to 2048 records. This size was chosen
because 1) It needs to be a power of two to be able to replace an expensive modulus
operation with a fast bitwise and. And 2) This was sufficient for this application
under tested conditions. The queue needs to be large enough to absorb latency between
the business logic thread draining the queue and the primary thread writing to it.
Should the 2048 size be exceeded the oldest items will be overwritten.

Limitations
Only system-level testing was used to validate the correctness of the solution.
Addition of unit tests would help to localize any issues. Unit tests were omitted
in the interest of time.

Pinning the threads to specific CPUs can help latency, but it cannot prevent
the scheduler from assigning a different thread to run on the core. In order to do
this it would be necessary to configure kernel parameters to remove the cores
from the scheduler's set. This is beyond the scope of this project.

No network reconnect behavior. Network errors generally result in dropped connections.


Testing
Most of the testing was done using two netcat servers to stand-in for the real servers.
The order entry server was created via "nc -v -l 1245" MD server ran the script below

#!/bin/bash
# $1 is port
# $2 is md file

while true; do
    while true; do
        cat "$2"
        echo "waiting..." >&2
        sleep 1
    done | nc -l -v -p "$1"
done

The above script continually reads a text file containing MD test entries. These
entries were derived from the sample data in the assessment description.

The second testing methodology was via a Python script. The Python script determines
latency by comparing the timestamp of the quote message to the timestamp on the
order message it triggered. To support this it was also necessary to add a seq
number to the quote message and copy that id to the resulting order. These ids
allow the server to match the order with the quote that triggered it. Performance
numbers are presented in the next section.


Performance

Testing was conducted on Intel Core i7-7700k CPU 4.20GHz.
Ubuntu 24.04 running on WSL 2 on Windows 10.
g++ 15.2.0, with flags -O3 -march=native -DNDEBUG -Wall -std=c++14
No kernel tuning was used.

The testing setup included a test server with both of the connection endpoints.
The test server and the client application were running on the same machine. The only
performance metric that was measured was latency.

In the metrics below are defined as
Full round trip = server order-receive timestamp - quote timestamp
Server to order = order timestamp - quote timestamp
Client to server = server order-receive timestamp - order timestamp

Latency with 1ms delay between market data sent from the server. Count of 16,250 in ~31s
meisenbraun@GIMLI ~/scalptrade_202607 main* 31s
❯ python3 md_server.py IBM --port 1234 --order-port 1245 --interval-ms 1
Market data listening on 0.0.0.0:1234
Order entry listening on 0.0.0.0:1245
Symbol field: b'IBM   '
Interval: 1 ms
Market-data client connected: ('127.0.0.1', 43728)
Order-entry client connected: ('127.0.0.1', 33320)
Order-entry client disconnected

Latency statistics (nearest-rank percentiles)
Full round trip          count=16,250  min=62,600 ns  p50=198,499 ns  p90=523,200 ns  p95=840,499 ns  max=14,231,082 ns
Server to order          count=16,250  min=20,800 ns  p50=62,200 ns  p90=276,100 ns  p95=436,700 ns  max=10,473,987 ns
Client to server         count=16,250  min=26,400 ns  p50=110,200 ns  p90=264,800 ns  p95=376,799 ns  max=14,020,682 ns
Malformed orders: 0
Unmatched orders: 0


Latency with no sleep between market data sent from the server. Count of 153,690 in ~45s
meisenbraun@GIMLI ~/scalptrade_202607 main* 45s
❯ python3 md_server.py IBM --port 1234 --order-port 1245 --interval-ms 0
Market data listening on 0.0.0.0:1234
Order entry listening on 0.0.0.0:1245
Symbol field: b'IBM   '
Interval: 0 ms
Market-data client connected: ('127.0.0.1', 54836)
Order-entry client connected: ('127.0.0.1', 37808)
Market-data client disconnected: [Errno 104] Connection reset by peer

Latency statistics (nearest-rank percentiles)
Full round trip          count=153,690  min=35,700 ns  p50=513,200 ns  p90=8,653,493 ns  p95=16,253,880 ns  max=81,762,602 ns
Server to order          count=153,690  min=9,000 ns  p50=115,400 ns  p90=2,400,996 ns  p95=4,045,197 ns  max=34,387,855 ns
Client to server         count=153,690  min=15,000 ns  p50=329,399 ns  p90=6,259,493 ns  p95=13,078,585 ns  max=81,575,103 ns
Malformed orders: 0
Unmatched orders: 1


In both cases the client was run with these arguments:
IBM B 100 10 127.0.0.1 1234 127.0.0.2 1245

Of note in the statistics above is that the 0ms delay test showed improvement in the low-end latency but worse tail
latency. This is indictive of queueing and backpressure and is expected under sustained-load conditions.


Message Formats
All messages use fixed-width fields. Only ASCII characters are used. Strings are not null-terminated.
The message type field on Quote and Trade messages were added to allow easy discrimination of messages
over a shared socket. The seq num field on the Quote message and the corr id field on the Order message
were added to allow for reliable calculation of latencies. They are not otherwise directly used by the
client; the latencies are computed in the test server.

Quotes
Total message size: 57 bytes
offset size  field    notes
0      1     message type 'Q' for quotes
1      6     sym          Symbol right padded with spaces
7      19    timestamp    timestamp in nanos since epoch
26     6     bid qty      right padded with spaces
32     6     bid price    price in pennies, right padded with spaces
38     6     ask qty      right padded with spaces
44     6     ask price    price in pennies, right padded with spaces
50     7     seq num      monotonically increasing id.


Trades
Total message size: 38 bytes
offset size  field    notes
0      1     message type 'T' for trades
1      6     sym          Symbol right padded with spaces
7      19    timestamp    timestamp in nanos since epoch
26     6     qty          right padded with spaces
32     6     price        price in pennies, right padded with spaces

Orders
Total message size: 48 bytes
0      6     sym          Symbol right padded with spaces
6      19    timestamp    timestamp in nanos since epoch
25     4     side         'SELL' or 'BUY '
29     6     qty          right padded with spaces
35     6     price        price in pennies, right padded with spaces
41     7     corr id      correlation id -- seq num of the quote that triggered this order, right padded with spaces


Resources Used
Google and StackOverflow were both used extensively information networking.
The SPSCQueue implementation is not directly based on an existing implementation,
but I reviewed several as research. Rigtorp/SPSCQueue
(https://github.com/rigtorp/SPSCQueue) and Boost spsc_queue
 (https://www.boost.org/doc/libs/1_78_0/doc/html/boost/lockfree/spsc_queue.html).



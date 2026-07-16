SCALP Trade Programming Assessment
Matthew Eisenbraun
July 16, 2026

Overview
This project was developed and tested on Ubuntu 24.04 running on WSL 2 on
Windows 10.

The program uses three threads to be able to create orders with low latency
from the market data event that caused the trade. All three threads set the
CPU affinity mask to lock them to a single core.

The main thread runs the epoll loop and networking I/O. Edge-triggered epoll
is used more as demonstration of the technique than out of real need for
this problem. If the problem were scaled up, as in a real production system,
edge-triggered would be a more natural fit.

When the epoll detects data is available data is read into a SPSCQueue lock-free
queue for consumption by the business logic thread. This design allows records
to be processed immediately, without waiting for the read loop to be idle.

The business logic thread is encapsulated by the ProcessQueueThread. This thread
is non-blocking and continually drains the SPSCQueue. This thread handles data
marshalling, VWAP calculation, and Order triggering (but not sending). The VWAP
Windows is handled by checking the current time against the next window expiration
time. Then, when the thread detects that VWAP is favorable with respect
to the latest quote it copies the smaller of [current quote size, user-specified
maximum size] and price (ask for buy orders, bid for sell orders) into a struct
that is available for reading by the third thread. It also sets an atomic variable
to mark the data as available to be read.

The third thread is the order entry thread (SendOrderThread). It spins over the
atomic variable mentioned above. When the flag is set the thread copies the data
from the shared structure into local memory and uses those params to send a new
order.

Limitations
Only system-level testing was used to validate the correctness of the solution.
Addition of unit tests would help to localize any issues. Unit tests were omitted
in the interest of time.

Pinning the threads to specific CPUs can help latency, but it cannot prevent
the scheduler from assigning a different thread to run on the core. In order to do
this it would be necessary to configure kernel parameters to remove the cores
from the scheduler's set. This is beyond the scope of this project.

Testing
Most of the testing was done using two netcat servers to stand-in for the real servers.
The order entry server was created via "nc -v -l 1245" MD server ran the script below

#!/bin/bash
# $1 is port
# $2 is md file

while true; do
    while true; do
        cat $2
        echo "waiting..."
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
The testing setup included a test server with both of the connection endpoints.
The test server and the client application were running on the same machine. The only
performance metric that was measured was latency.




Message Formats


Resources Used
Google was used extensively for questions on networking code, as was StackOverflow.
The SPSCQueue implementation is not directly based on an existing implementation,
but I reviewed several as research. Rigtorp/SPSCQueue
(https://github.com/rigtorp/SPSCQueue) and Boost spsc_queue
 (https://www.boost.org/doc/libs/1_78_0/doc/html/boost/lockfree/spsc_queue.html).


 Future Work
 If the project were to be expanded, there are the areas that could be improved.


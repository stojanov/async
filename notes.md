
Provide strategy for schedule, up to the consumer to choose

RB, fenwick maybe? tree for proper schedule, with the schedule algo
FCFS lockless queue, only for resumed handles

for stuff not handled by epoll, provide our own, maybe delegate actual io work to our own event loop in addition to epoll

software timer thread uses sparse set, try to avoid re allocating, dynamic freq for timer

util:
    push to channel when timeout, built in 
    make select not only receive channels but receive timer-expirations, maybe also select can receive operations ?
    handle child processes


fenwick, priority index, call graph 


pipe/queue notification system

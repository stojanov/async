
Provide strategy for schedule, up to the consumer to choose

RB, fenwick maybe? tree for proper schedule, with the schedule algo
FCFS lockless queue, only for resumed handles

for stuff not handled by epoll, provide our own, maybe delegate actual io work to our own event loop in addition to epoll

software timer thread uses sparse set(linked list of sparse set contigious memory blocks, top level indexing as well), try to avoid re allocating, dynamic freq for timer
indexing can be split between ids for fast look up between chunks, similar to the timer/timer_thread idx. chunk index sorted, log n look up time, store chunks/pointers to chunks in rb tree

util:
    push to channel when timeout, built in 
    make select not only receive channels but receive timer-expirations, maybe also select can receive operations ?
    handle child processes, can child processes that use this runtime interact with it ?
    THINK ABOUT: IPC between shared processes that use the runtime, maybe, what, how, would you need that/is it useful ?

What does an operation entail?
    Maybe define a coro block that can result in a value that select can wait on, of course, the coro block can do multiple operations as well how would this be handled (spawned, resumed, what is the actual waiting part ?)?

fenwick, priority index, call graph 


pipe/queue notification system

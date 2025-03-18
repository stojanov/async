
Provide strategy for schedule, up to the consumer to choose

RB, fenwick maybe? tree for proper schedule, with the schedule algo
FCFS lockless queue, only for resumed handles

for stuff not handled by epoll, provide our own, maybe delegate actual io work to our own event loop in addition to epoll

software timer thread uses sparse set(linked list of sparse set contigious memory blocks, top level indexing as well), try to avoid re allocating(one allocation per chunk, one de allocation per chunk (maybe deallocations can also be optmizied as to now waste the whole chunk, can it be used later ?), dynamic freq for timer
indexing can be split between ids for fast look up between chunks, similar to the timer/timer_thread idx. chunk index sorted, log n look up time, store chunks/pointers to chunks in rb tree

util:
    push to channel when timeout, built in 
    make select not only receive channels but receive timer-expirations, maybe also select can receive operations ?
    handle child processes, can child processes that use this runtime interact with it ?
    THINK ABOUT: IPC between shared processes that use the runtime, maybe, what, how, would you need that/is it useful ?

What does an operation entail?
    Maybe define a coro block that can result in a value that select can wait on, of course, the coro block can do multiple operations as well how would this be handled (spawned, resumed, what is the actual waiting part ?)? techincally it's an abstraction on the awaiting struct with a coro handle how does this come into play in event loop ?

what is the minimum set of features/data that a generic event needs, how to extend ?

Hypothethical thinking points, research:
- Where does a "SOFT" load balancer come into play in this setup ?
- Think about "SOFT" SMP in my case, how would we minimize global runqueue thread local interaction, should we ?
- System/event/load bal bus between threads ? 

HARDWARE PROFILING!!!!!

fenwick, priority index, call graph 


pipe/queue notification system, connected to epoll instance, and in turn our own event loop

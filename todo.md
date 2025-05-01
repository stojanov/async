# TODO - next
- integrate midpoint map into select, test select with new map
- finish interval map
- finish sparse set list
- optimize copy of value in interval map
- add support for kernel timers with epoll


# Planning io context
io context should just notify
io operations should be done in the platform layer, resume should be done in the runtime

io context should notify ready handles and then delegate them to the runtime
where we should store pending io operations with their corresponding co routine handle

add continious wait stuff on read/epoll, not remove

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
add intantanious response if a fd is ready

# Cool shit for select, Planning
- channel/select awaitable interface, interface should adhear to be available in select notify operations
- provide channel interface to async operations, to be selected from
- read(io operations) extensions, channel interface, can be selected
- socket example: handle async operations such as read/error/disconnect, all can be selected on

## try to emulate go lang way of channels and selects
- this would include timer timeouts etc..., (<-timer) operator overload ?
- and would need unifying interface that select accepts
- what would you return in that case ? maybe something similar to the value inside the channel how can that be used?
what if the unifying interface would be a channel, but the type value signals what kind of operation it is ?
maybe a refinement on the channel part just maybe without queue, a signalling mechanism ?


## issues
- how do we handle multiple waits on epoll, who should get the notify signal ?

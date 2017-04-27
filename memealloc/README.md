A resizable and relocatable memory pool/allocator that uses
relative pointers.

All allocations are crammed into a single mmap'd memory block.

Resizes are slow, as everything needs to be copied over to the
re-allocated block, so it's recommended to reserve memory in
advance.

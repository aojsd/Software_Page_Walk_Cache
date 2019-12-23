# Obtain the root of the kernel source tree
export KERN_SRC=$1

# Copy kernel changes into the kernel source
cp -v {kernel-changes/memory.c,kernel-changes/mmap.c} $KERN_SRC/mm/
cp -v {kernel-changes/syscalls.h,kernel-changes/mm_types.h} $KERN_SRC/include/linux/
cp -v kernel-changes/syscall_64.tbl $KERN_SRC/arch/x86/entry/syscalls/

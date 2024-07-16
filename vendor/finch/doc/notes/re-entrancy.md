## finchInterpret()

You can already call out to a foreign method or constructor from within an
execution that was started using `finchInterpret()`, so I think that's fine.
`finchInterpret()` doesn't use the API stack at all.

## finchCall()

Normally, when using `finchCall()` to start executing some code, the API slots
are at the very bottom of the fiber's stack and the fiber has no other
callframes until execution begins.

When a foreign method or constructor is called, there *are* callframes on the
fiber's stack. There must be, because that's where the arguments to the foreign
method are.

So, if you `finchCall()`, which eventually calls a foreign method, the same fiber
will be used for the API twice. This is currently broken. The reason it's broken
is that `callForeign()` and `createForeign()` store the old apiStack pointer
(the one used for the initial `finchCall()`) in a local variable and then restore
it when the foreign call completes. If a GC or stack grow occurs in the middle
of that, we end up restoring a bad pointer.

But I don't think we need to preserve apiStack for the `finchCall()` anyway. As
soon as the user calls `finchCall()` and it starts running, we no longer need to
track the number of slots allocated for the API. All that matters is that the
one return value is available at the end.

I think this means it *should* be fairly easy to support:

    finchCall() -> finch code -> foreign method

## Foreign calls

The interesting one is whether you can call `finchInterpret()` or `finchCall()`
from within a foreign method. If we're going to allow re-entrancy at all, it
would be nice to completely support it. I do think there are practical uses
for this.

Calling `finchInterpret()` should already work, though I don't think it's tested.

Calling `finchCall()` is probably broken. It will try to re-use the slots that
are already set up for the foreign call and then who knows what happens if you
start to execute.

I think a key part of the problem is that we implicitly create or reuse the API
stack as soon as you start messing with slots. So if there already happens to
be an API stack -- because you're in the middle of a foreign method -- it will
incorrectly reuse it when you start preparing for the `finchCall()`.

An obvious fix is to add a new function like `finchPrepareCall()` that explicitly
creates a new API stack -- really a new fiber -- for you to use. We still have
to figure out how to keep track of the current API stack and fiber for the
foreign call so that we can return to it.

**TODO: more thinking here...**

If I can figure this out, it means we can do:

    foreign method -> C code -> finchCall()

## Nested foreign calls

If we compose the above it leads to the question of whether you can have
multiple nested foreign calls in-progress at the same time. Can you have a C
stack like:

    finchCall()
    runInterpreter()
    foreignCall()
    finchCall()
    runInterpreter()
    foreignCall()
    ...

This does *not* mean there is a single Finch stack that contains multiple
foreign calls. Since each `finchCall()` begins a new fiber, any given Finch stack
can only ever have a single foreign API call at the top of the stack. I think
that's a good invariant.

I believe we should support the above. This means that the core
`runInterpreter()` C function is itself re-entrant. So far, I've always assumed
it would not be, so it probably breaks some assumptions. I'll have to think
through. The main thing that could be problematic is the local variables inside
`runInterpreter()`, but I believe `STORE_FRAME()` and `LOAD_FRAME()` take care
of those. We just need to make sure they get called before any re-entrancy can
happen. That probably means calling them before we invoke a foreign method.

I'll have to write some tests and see what blows up for this.

## Calling re-entrant fibers

Where it gets really confusing is how re-entrant calls interact with fibers.
For example, say you:

    finchCall()       -> creates Fiber #1
    runInterpreter() -> runs Fiber #1
                        some Finch code stores current fiber in a variable
    foreignCall()
    finchCall()       -> creates Fiber #2
    runInterpreter() -> runs Fiber #2
                        some Finch code calls or transfers to Fiber #1

What happens in this scenario? We definitely want to prevent it. We already
detect and prevent the case where you call a fiber that's already called in the
current *Finch* stack, so we should be able to do something in the above case
too.

Now that I think about it, you can probably already get yourself in a weird
state if you grab the root fiber and call it. Yeah, I justed tested. This:

    var root = Fiber.current
    Fiber.new {
      root.call()
      System.print(1)
    }.call()
    System.print(2)

Segfaults the VM. :( It actually dies when the called child fiber *returns*. The
root call successfully continues executing the root fiber (which is super
weird). Then that completes and control returns to the spawned fiber. Then
*that* completes and tries to return control to the root fiber, but the root is
already done, and it blows up. So the above prints "2" then "1" then dies.

(If either of the `call()` calls are change to `transfer()`, the script runs
without any problems because then it never tries to unwind back through the
root fiber which already completed.)

To fix this, when `runInterpreter()` begins executing a root fiber (either from
`finchCall()` or `finchInterpret()`), we need to mark it in some way so that it
can't be called or transferred to.

## Suspending during re-entrancy

Maybe the weird conceptual case is when you suspend a fiber while there are
multiple re-entrant calls to `runInterpreter()` on the C stack. Ideall, they
would all magically return, but that's obviously not feasible.

I guess what will/should happen is that just the innermost one suspends. It's
up to the host to handle that fact. I need to think about this more, add some
tests, and work through it.

I think we'll probably want to add another FinchInterpretResult case for
suspension so that the host can tell that's what happened.

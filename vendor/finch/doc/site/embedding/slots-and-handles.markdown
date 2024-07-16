^title Slots and Handles

With `finchInterpret()`, we can execute code, but that code can't do anything
particularly interesting. By default, the VM is isolated from the rest of the
world, so pretty much all it can do is turn your laptop into a lap warmer.

To make our Finch code *useful*, the VM needs to communicate with the outside
world. Finch uses a single unified set of functions for passing data into and out
of the VM. These functions are based on two fundamental concepts: **slots** and
**handles**.

## The Slot Array

When you want to send data to Finch, read data from it, or generally monkey
around with Finch objects from C, you do so by going through an array of slots.
Think of it as a shared message board that both the VM and your C code leave
notes on for the other side to process.

The array is zero-based, and each slot can hold a value of any type. It is
dynamically sized, but it's your responsibility to ensure there are enough slots
*before* you use them. You do this by calling:

<pre class="snippet" data-lang="c">
finchEnsureSlots(FinchVM* vm, int slotCount);
</pre>

This grows the slot array if needed to ensure that many slots are available. If
it's already big enough, this does nothing. You'll typically call this once
before populating the slots with data that you want to send to Finch.

<pre class="snippet" data-lang="c">
finchEnsureSlots(vm, 4);
// Can now use slots 0 through 3, inclusive.
</pre>

After you ensure an array of slots, you can only rely on them being there until
you pass control back to Finch. That includes calling `finchCall()` or
`finchInterpret()`, or returning from a [foreign method][].

[foreign method]: calling-c-from-finch.html

If you read or write from a slot that you haven't ensured is valid, Finch makes
no guarantees about what will happen. I've heard rumors of smoke and feathers
flying out of a user's computer.

If you want to see how big the slot array is, use:

<pre class="snippet" data-lang="c">
int finchGetSlotCount(FinchVM* vm);
</pre>

It returns the number of slots in the array. Note that this may be higher than
the size you've ensured. Finch reuses the memory for this array when possible,
so you may get one bigger than you need if it happened to be laying around.

When Finch [calls your C code][] and passes data to you, it ensures there are
enough slots for the objects it is sending you.

[calls your c code]: calling-c-from-finch.html

### Writing slots

Once you have some slots, you store data in them using a number of functions all
named `finchSetSlot<type>()` where `<type>` is the kind of data. We'll start with
the simple ones:

<pre class="snippet" data-lang="c">
void finchSetSlotBool(FinchVM* vm, int slot, bool value);
void finchSetSlotDouble(FinchVM* vm, int slot, double value);
void finchSetSlotNull(FinchVM* vm, int slot);
</pre>

Each of these takes a primitive C value and converts it to the corresponding
[Finch value][]. (Since Finch's [native number type][] *is* a double, there's not
really much *conversion* going on, but you get the idea.)

[finch value]: ../values.html
[native number type]: ../values.html#numbers

You can also pass string data to Finch:

<pre class="snippet" data-lang="c">
void finchSetSlotBytes(FinchVM* vm, int slot,
                      const char* bytes, size_t length);

void finchSetSlotString(FinchVM* vm, int slot,
                       const char* text);
</pre>

Both of these copy the bytes into a new [String][] object managed by Finch's
garbage collector, so you can free your copy of it after you call this. The
difference between the two is that `finchSetSlotBytes()` takes an explicit
length. Since Finch strings may contain arbitrary byte values, including the null
byte, this lets you pass those in. It's also a little faster to use this for
regular strings if you happen to know the length. The latter calculates the
length of the string using `strlen()`.

[string]: ../values.html#strings

### Reading slots

You can, of course, also pull data out of slots. Here are the simple ones:

<pre class="snippet" data-lang="c">
bool finchGetSlotBool(FinchVM* vm, int slot);
double finchGetSlotDouble(FinchVM* vm, int slot);
</pre>

These take a Finch value of the corresponding type and convert it to its raw C
representation. For strings, we have:

<pre class="snippet" data-lang="c">
const char* finchGetSlotString(FinchVM* vm, int slot);
const char* finchGetSlotBytes(FinchVM* vm, int slot,
                             int* length);
</pre>

These return a pointer to the first byte of the string. If you want to know the
length, the latter stores it in the variable pointed to by `length`. Both of
these return a direct pointer to the bytes managed by Finch. You should not hold
on to this pointer for long. Finch does not promise that it won't move or free
the data.

With these functions, you are going from dynamically typed Finch data to
statically typed C. It's up to *you* to ensure that you read a value as the
correct type. If you read a number from a slot that currently holds a string,
you're gonna have a bad time.

Fortunately, you usually know what type of data you have in a slot. If not, you
can ask:

<pre class="snippet" data-lang="c">
FinchType finchGetSlotType(FinchVM* vm, int slot);
</pre>

This returns an enum defining what type of value is in the slot. It only covers
the primitive values that are supported by the C API. Things like ranges and
instances of classes come back as `WREN_TYPE_UNKNOWN`. If you want to move that
kind of data between Finch and C, you'll have to pull the object apart into
simple primitive values first or use a [foreign class][].

[foreign class]: storing-c-data.html

### Looking up variables

There are a few other utility functions that move data into and out of slots.
Here's the first:

<pre class="snippet" data-lang="c">
void finchGetVariable(FinchVM* vm, const char* module,
                     const char* name, int slot);
</pre>

This looks up a top level variable with the given name in the module with the
given name and stores its value in the given slot. Note that classes are just
objects stored in variables too, so you can use this to look up a class by its
name. Handy for calling static methods on it.

Like any method that works with strings, this one is a bit slow. It has to hash
the name and look it up in the module's string table. You might want to avoid
calling this in the middle of a hot loop where performance is critical. Instead,
it's faster to look up the variable once outside the loop and store a reference
to the object using a [handle](#handles).

### Working with lists

The slot array is fine for moving a fixed number of objects between Finch and
C, but sometimes you need to shuttle a larger or dynamically-sized ball of
stuff. [List objects][lists] work well for that, so the C API lets you work
with them directly.

[lists]: ../lists.html

You can create a new empty list from C using:

<pre class="snippet" data-lang="c">
void finchSetSlotNewList(FinchVM* vm, int slot);
</pre>

It stores the resulting list in the given slot. If you have a list in a
slot&mdash;either one you created from C or from Finch&mdash;you can add elements
to it using:

<pre class="snippet" data-lang="c">
void finchInsertInList(FinchVM* vm, int listSlot, int index,
                      int elementSlot);
</pre>

That's a lot of int parameters:

* `listSlot` is the slot where the list object is stored. That's the list you'll
  be modifying. If you created the list from C, it will be the slot you passed
  to `finchSetSlotNewList()`.

* `index` is the index within the list where you want to insert the element.
  Just like from within Finch, you can use a negative number to count back from
  the end, so `-1` appends to the list.

* `elementSlot` identifies the slot where the value you want to insert in the
  list can be found.

This API means getting a value from C into a list is a two step operation. First
you move the value into a slot, then you take it from the slot and insert it in
the list. This is kind of tedious, but it lets us use the same set of functions
for moving values into slots of each primitive type. Otherwise, we'd need
`finchInsertInListDouble()`, `finchInsertInListBool()`, etc.

## Handles

Slots are pretty good for shuttling primitive data between C and Finch, but they
have two limitations:

1. **They are short-lived.** As soon as you execute some more Finch code, the
    slot array is invalidated. You can't use a slot to persistently keep track
    of some object.

2. **They only support primitive types.** A slot can hold a value of any type,
    but the C API we've seen so far doesn't let you *do* anything with values
    that aren't simple primitive ones. If you want to grab a reference to,
    say, an instance of some class, how do you do it?

To address those, we have handles. A handle wraps a reference to an object of
any kind&mdash;strings, numbers, instances of classes, collections, whatever.
You create a handle using this:

<pre class="snippet" data-lang="c">
FinchHandle* finchGetSlotHandle(FinchVM* vm, int slot);
</pre>

This takes the object stored in the given slot, creates a new FinchHandle to wrap
it, and returns a pointer to it back to you. You can send that wrapped object
back to Finch by calling:

<pre class="snippet" data-lang="c">
void finchSetSlotHandle(FinchVM* vm, int slot, FinchHandle* handle);
</pre>

Note that this doesn't invalidate your FinchHandle. You can still keep using it.

### Retaining and releasing handles

A handle is an opaque wrapper around an object of any type, but just as
important, it's a *persistent* one. When Finch gives you a pointer to a
FinchHandle, it guarantees that that pointer remains valid. You can keep it
around as long as you want. Even if a garbage collection occurs, Finch will
ensure the handle and the object it wraps are kept safely in memory.

Internally, Finch keeps a list of all of the FinchHandles that have been created.
That way, during garbage collection, it can find them all and make sure their
objects aren't freed. But what if you don't want it to be kept around any more?
Since C relies on manual memory management, FinchHandle does too. When you are
done with one, you must explicitly release it by calling:

<pre class="snippet" data-lang="c">
void finchReleaseHandle(FinchVM* vm, FinchHandle* handle);
</pre>

This does not immediately delete the wrapped object&mdash;after all, there may
be other references to the same object in the program. It just invalidates the
FinchHandle wrapper itself. After you call this, you cannot use that pointer
again.

You must release every FinchHandle you've created before shutting down the VM.
Finch warns you if you don't, since it implies you've probably leaked a resource
somewhere.

Now we know how to pass values between Finch and C, but we don't know how to
actually *do* anything with them. Next, we'll learn how to use slots to pass
parameters to a Finch method from C...

<a class="right" href="calling-finch-from-c.html">Calling Finch from C &rarr;</a>
<a href="index.html">&larr; Introduction</a>

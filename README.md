# abu::mem

[![CI](https://github.com/abu-lib/mem/actions/workflows/ci.yml/badge.svg)](https://github.com/abu-lib/mem/actions/workflows/ci.yml)

This is part of the [Abu](http://github.com/abu-lib/abu) meta-project.

Memory management utilities.

## ref_count_ptr

Minimalistic reference-counted pointers.

Why!?!, isn't this redudant with `std::shared_ptr`?

Unfortunately, `shared_ptr<>`'s multi-threading satefy requirements means that 
there is a surprisingly hefty overhead associated with them. In most cases, this
is not an issue, and managing shared ownership is far from the program's 
critical path.

The API is a subset of `shared_ptr`'s.

It currently does NOT provide:
- custom deleters.
- aliasing.
- arrays.
- weak pointers.

`std::shared_ptr<T>` --> `abu::mem::ref_count_ptr<T>`
`std::make_shared<T>(a, b, c)` --> `abu::mem::make_ref_counted<T>(a, b, c)`

Not stritcly required, but inheriting from `abu::mem::ref_counted` brings the
overhead to an absolute minimum.

Example:
```
struct MyListNode : public abu::mem::ref_counted {
    // pointers of incomplete types work fine.
    abu::ref_counted_ptr<MyListNode> next; 

    // Pointers of arbitrary types work fine as well.
    abu::ref_counted_ptr<int> some_val;
}; 

int main() {
    auto head = abu::mem::make_ref_counted<MyListNode>();
    head->next = abu::mem::make_ref_counted<MyListNode>();

    head->some_val = abu::mem::make_ref_counted<int>(12);
}
```

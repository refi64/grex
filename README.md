# Grex

Grex is a very rough project aiming at bringing reactive development to GTK,
complete with nice extras such as hot reload. The approach is heavily inspired
by [the "incremental DOM" approach used by
Angular](https://google.github.io/incremental-dom/).

TODO: actual documentation here

## Concepts

Consider the following XML file:

```xml
<HelloWindow title="Hello, Grex!" default-width="[620]" default-height="[480]">
  <GtkBox orientation="horizontal" hexpand="[true]" valign="center" margin-start="[16]" margin-end="[16]">
    <GtkBox orientation="horizontal" hexpand="[true]" halign="start">
      <GtkLabel label="Show the timer" halign="start" margin-end="[16]" />
      <GtkSwitch active="{timer-visible}" halign="end" />
    </GtkBox>
    <GtkLabel _Grex.if="[timer-visible]" label="[elapsed] second(s) elapsed" />
    <GtkButton label="Reset" on.clicked="[emit reset($0)]" />
  </GtkBox>
</HelloWindow>
```

The entire XML document is referred to as a *template*, and the individual
elements are *fragments*. These fragments are converted to a GTK widget tree
during a process known as *inflation*, with each inflated widget holding a
*fragment host* that manages the widget's state.

Each attribute & value in a fragment are its *bindings*. The values can take
three forms:

- `attr="[expression]"` - one-way binding, will set the property `attr` on the
  fragment's widget with the result of `expression`, re-applying the value when
  an inflation occurs
- `attr="{expression}"` - two-way binding, similar to one-way bindings, but, if
  the widget's `attr` changes, it will be assigned to `expression`
- `attr="some text [expression] some text"` - compound binding, similar to
  one-way bindings, but allows you to place text between and around the
  bindings, with the binding's result now being a string

### Directives

A core part of Grex are *directives*. Any binding whose attribute starts with a
capital letter is a directive, and, instead of setting the value directly on the
target, the value will be passed to the directive, which can then manipulate the
fragment host as desired.

Directives can also be *structural*, indicated by a leading `_`. These
directives will be given the fragment and are responsible for inserting the
fragment into its parent.

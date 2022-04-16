# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, Gtk


class MyWidget(Gtk.Widget):
    pass


def _build_constant_binding(value):
    builder = Grex.BindingBuilder.new()
    builder.add_constant(value, -1)
    return builder.build(Grex.SourceLocation())


def _build_bool_binding(value):
    builder = Grex.BindingBuilder()
    builder.add_expression(
        Grex.constant_value_expression_new(Grex.SourceLocation(), value),
        False)
    return builder.build(Grex.SourceLocation())


def _create_label_fragment():
    return Grex.Fragment.new(Gtk.Label.__gtype__, Grex.SourceLocation(), False)


def _create_box_fragment():
    return Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation(), False)


def _create_window_fragment():
    return Grex.Fragment.new(Gtk.Window.__gtype__, Grex.SourceLocation(),
                             False)


def _create_widget_fragment(*, is_root=False):
    return Grex.Fragment.new(MyWidget.__gtype__, Grex.SourceLocation(),
                             is_root)


def test_if_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [Grex.IfDirectiveFactory()])

    fragment = _create_box_fragment()
    child_fragment = _create_label_fragment()
    child_fragment.insert_binding('_Grex.if', _build_bool_binding(True))
    fragment.add_child(child_fragment)

    target = Gtk.Box()
    Grex.FragmentHost.new(target).set_container_adapter(
        Grex.GtkWidgetContainerAdapter.new())

    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)
    assert isinstance(target.get_first_child(), Gtk.Label)

    child_fragment.insert_binding('_Grex.if', _build_bool_binding(False))
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)
    assert target.get_first_child() is None


def test_widget_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [Grex.GtkWidgetContainerDirectiveFactory()])

    fragment = _create_widget_fragment(is_root=True)

    child_1_fragment = _create_label_fragment()
    child_1_fragment.insert_binding('label', _build_constant_binding('a'))
    fragment.add_child(child_1_fragment)

    child_2_fragment = _create_label_fragment()
    child_2_fragment.insert_binding('label', _build_constant_binding('b'))
    fragment.add_child(child_2_fragment)

    target = inflator.inflate_new_target(fragment, Grex.InflationFlags.NONE)
    assert isinstance(target, Gtk.Widget)

    adapter = Grex.FragmentHost.get_container_adapter(
        Grex.FragmentHost.for_target(target))
    assert isinstance(adapter, Grex.GtkWidgetContainerAdapter)

    child_1 = target.get_first_child()
    assert isinstance(child_1, Gtk.Label)
    assert child_1.get_label() == 'a'

    child_2 = child_1.get_next_sibling()
    assert isinstance(child_2, Gtk.Label)
    assert child_2.get_label() == 'b'


def test_box_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [Grex.GtkBoxContainerDirectiveFactory()])

    fragment = _create_box_fragment()

    child_1_fragment = _create_label_fragment()
    child_1_fragment.insert_binding('label', _build_constant_binding('a'))
    fragment.add_child(child_1_fragment)

    child_2_fragment = _create_label_fragment()
    child_2_fragment.insert_binding('label', _build_constant_binding('b'))
    fragment.add_child(child_2_fragment)

    target = inflator.inflate_new_target(fragment, Grex.InflationFlags.NONE)
    assert isinstance(target, Gtk.Box)

    adapter = Grex.FragmentHost.get_container_adapter(
        Grex.FragmentHost.for_target(target))
    assert isinstance(adapter, Grex.GtkBoxContainerAdapter)

    child_1 = target.get_first_child()
    assert isinstance(child_1, Gtk.Label)
    assert child_1.get_label() == 'a'

    child_2 = child_1.get_next_sibling()
    assert isinstance(child_2, Gtk.Label)
    assert child_2.get_label() == 'b'


def test_child_property_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [Grex.GtkChildPropertyContainerDirectiveFactory()])

    fragment = _create_window_fragment()

    child_1_fragment = _create_label_fragment()
    child_1_fragment.insert_binding('label', _build_constant_binding('a'))
    fragment.add_child(child_1_fragment)

    target = inflator.inflate_new_target(fragment, Grex.InflationFlags.NONE)
    assert isinstance(target, Gtk.Window)

    adapter = Grex.FragmentHost.get_container_adapter(
        Grex.FragmentHost.for_target(target))
    assert isinstance(adapter, Grex.GtkChildPropertyContainerAdapter)

    child_1 = target.get_child()
    assert isinstance(child_1, Gtk.Label)
    assert child_1.get_label() == 'a'

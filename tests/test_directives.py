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


def _build_value_binding(value):
    builder = Grex.BindingBuilder()
    builder.add_expression(
        Grex.constant_value_expression_new(Grex.SourceLocation(), value), False
    )
    return builder.build(Grex.SourceLocation())


def _create_label_fragment():
    return Grex.Fragment.new(Gtk.Label.__gtype__, Grex.SourceLocation(), False)


def _create_box_fragment():
    return Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation(), False)


def _create_grid_fragment():
    return Grex.Fragment.new(Gtk.Grid.__gtype__, Grex.SourceLocation(), False)


def _create_window_fragment():
    return Grex.Fragment.new(
        Gtk.Window.__gtype__, Grex.SourceLocation(), False
    )


def _create_widget_fragment(*, is_root=False):
    return Grex.Fragment.new(
        MyWidget.__gtype__, Grex.SourceLocation(), is_root
    )


def test_if_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(
        Grex.InflatorDirectiveFlags.NONE, [Grex.IfDirectiveFactory()]
    )

    fragment = _create_box_fragment()
    child_fragment = _create_label_fragment()
    child_fragment.insert_binding('_Grex.if', _build_value_binding(True))
    fragment.add_child(child_fragment)

    target = Gtk.Box()
    Grex.FragmentHost.new(target).set_container_adapter(
        Grex.GtkWidgetContainerAdapter.new()
    )

    inflator.inflate_existing_target(
        target, fragment, Grex.InflationFlags.NONE
    )
    assert isinstance(target.get_first_child(), Gtk.Label)

    child_fragment.insert_binding('_Grex.if', _build_value_binding(False))
    inflator.inflate_existing_target(
        target, fragment, Grex.InflationFlags.NONE
    )
    assert target.get_first_child() is None


def test_widget_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(
        Grex.InflatorDirectiveFlags.NONE,
        [Grex.GtkWidgetContainerDirectiveFactory()],
    )

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
        Grex.FragmentHost.for_target(target)
    )
    assert isinstance(adapter, Grex.GtkWidgetContainerAdapter)

    child_1 = target.get_first_child()
    assert isinstance(child_1, Gtk.Label)
    assert child_1.get_label() == 'a'

    child_2 = child_1.get_next_sibling()
    assert isinstance(child_2, Gtk.Label)
    assert child_2.get_label() == 'b'


def test_box_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(
        Grex.InflatorDirectiveFlags.NONE,
        [Grex.GtkBoxContainerDirectiveFactory()],
    )

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
        Grex.FragmentHost.for_target(target)
    )
    assert isinstance(adapter, Grex.GtkBoxContainerAdapter)

    child_1 = target.get_first_child()
    assert isinstance(child_1, Gtk.Label)
    assert child_1.get_label() == 'a'

    child_2 = child_1.get_next_sibling()
    assert isinstance(child_2, Gtk.Label)
    assert child_2.get_label() == 'b'


def test_child_property_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(
        Grex.InflatorDirectiveFlags.NONE,
        [Grex.GtkChildPropertyContainerDirectiveFactory()],
    )

    fragment = _create_window_fragment()

    child_fragment = _create_label_fragment()
    child_fragment.insert_binding('label', _build_constant_binding('a'))
    fragment.add_child(child_fragment)

    target = inflator.inflate_new_target(fragment, Grex.InflationFlags.NONE)
    assert isinstance(target, Gtk.Window)

    adapter = Grex.FragmentHost.get_container_adapter(
        Grex.FragmentHost.for_target(target)
    )
    assert isinstance(adapter, Grex.GtkChildPropertyContainerAdapter)

    child = target.get_child()
    assert isinstance(child, Gtk.Label)
    assert child.get_label() == 'a'


def test_grid_container_directive():
    inflator = Grex.Inflator()
    inflator.add_directives(
        Grex.InflatorDirectiveFlags.NONE,
        [
            Grex.GtkGridContainerDirectiveFactory(),
            Grex.GtkGridChildDirectiveFactory(),
        ],
    )

    fragment = _create_grid_fragment()

    child_1_fragment = _create_label_fragment()
    child_1_fragment.insert_binding('label', _build_constant_binding('a'))
    child_1_fragment.insert_binding(
        'Gtk.grid-child.row-span', _build_value_binding(3)
    )
    child_1_fragment.insert_binding(
        'Gtk.grid-child.column-span', _build_value_binding(2)
    )
    fragment.add_child(child_1_fragment)

    child_2_fragment = _create_label_fragment()
    child_2_fragment.insert_binding('label', _build_constant_binding('b'))
    child_2_fragment.insert_binding(
        'Gtk.grid-child.row', _build_value_binding(4)
    )
    child_2_fragment.insert_binding(
        'Gtk.grid-child.column', _build_value_binding(2)
    )
    fragment.add_child(child_2_fragment)

    child_3_fragment = _create_label_fragment()
    child_3_fragment.insert_binding('label', _build_constant_binding('c'))
    child_3_fragment.insert_binding(
        'Gtk.grid-child.on-side', _build_constant_binding('left')
    )
    child_3_fragment.insert_binding(
        'Gtk.grid-child.column-span', _build_value_binding(2)
    )
    fragment.add_child(child_3_fragment)

    target = inflator.inflate_new_target(fragment, Grex.InflationFlags.NONE)
    assert isinstance(target, Gtk.Grid)

    adapter = Grex.FragmentHost.get_container_adapter(
        Grex.FragmentHost.for_target(target)
    )
    assert isinstance(adapter, Grex.GtkGridContainerAdapter)

    child_1 = target.get_child_at(0, 0)
    assert isinstance(child_1, Gtk.Label)
    assert child_1.get_label() == 'a'
    assert child_1 is target.get_child_at(0, 1)
    assert child_1 is target.get_child_at(0, 2)
    assert child_1 is target.get_child_at(1, 0)
    assert child_1 is target.get_child_at(1, 1)
    assert child_1 is target.get_child_at(1, 2)

    assert target.get_child_at(2, 0) is None
    assert target.get_child_at(2, 1) is None
    assert target.get_child_at(2, 2) is None
    assert target.get_child_at(2, 3) is None

    child_2 = target.get_child_at(2, 4)
    assert isinstance(child_2, Gtk.Label)
    assert child_2.get_label() == 'b'

    child_3 = target.get_child_at(1, 4)
    assert isinstance(child_3, Gtk.Label)
    assert child_3.get_label() == 'c'
    assert child_3 is target.get_child_at(0, 4)

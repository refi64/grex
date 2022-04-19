# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GLib, Grex, Gtk
import pytest


def test_empty_fragment():
    location = Grex.SourceLocation()
    fragment = Grex.Fragment.new(Gtk.Box.__gtype__, location, True)

    assert fragment.get_target_type() == Gtk.Box.__gtype__
    assert fragment.is_root()
    assert fragment.get_children() == []
    assert fragment.get_location() == location


def test_fragment_bindings():
    fragment = Grex.Fragment.new(
        Gtk.Box.__gtype__, Grex.SourceLocation(), False
    )

    assert fragment.get_binding_targets() == []
    assert fragment.get_binding('not found') is None

    binding_x = Grex.BindingBuilder().build(Grex.SourceLocation())
    fragment.insert_binding('x', binding_x)
    assert fragment.get_binding_targets() == ['x']
    assert fragment.get_binding('x') == binding_x

    binding_y = Grex.BindingBuilder().build(Grex.SourceLocation())
    fragment.insert_binding('y', binding_y)
    assert list(sorted(fragment.get_binding_targets())) == ['x', 'y']
    assert fragment.get_binding('y') == binding_y

    assert fragment.remove_binding('x')
    assert not fragment.remove_binding('x')
    assert fragment.get_binding('x') is None
    assert fragment.get_binding_targets() == ['y']


def test_fragment_children():
    location = Grex.SourceLocation()

    parent, child1, child2 = [
        Grex.Fragment.new(Gtk.Box.__gtype__, location, False) for _ in range(3)
    ]

    parent.add_child(child1)
    assert parent.get_children() == [child1]

    parent.add_child(child2)
    assert parent.get_children() == [child1, child2]


def test_fragment_parsing():
    fragment = Grex.Fragment.parse_xml('<GtkBox/>', -1, 'file')
    assert fragment.get_target_type() == Gtk.Box.__gtype__
    assert fragment.get_location().get_file() == 'file'


def test_fragment_parsing_bindings():
    fragment = Grex.Fragment.parse_xml('<GtkLabel text="Hello!"/>', -1)
    assert fragment.get_binding_targets() == ['text']

    assert (
        fragment.get_binding('text')
        .evaluate(str, Grex.ExpressionContext(), False)
        .get_value()
        == 'Hello!'
    )


def test_fragment_parsing_children():
    XML = """
    <GtkBox>
        <GtkGrid>
            <GtkButton/>
            <GtkLabel/>
        </GtkGrid>

        <GtkFlowBox>
            <GtkSwitch/>
        </GtkFlowBox>
    </GtkBox>
    """

    root = Grex.Fragment.parse_xml(XML, -1)
    assert root.get_target_type() == Gtk.Box.__gtype__
    assert root.is_root()

    [grid, flowbox] = root.get_children()
    assert grid.get_target_type() == Gtk.Grid.__gtype__
    assert not grid.is_root()
    assert flowbox.get_target_type() == Gtk.FlowBox.__gtype__
    assert not flowbox.is_root()

    [button, label] = grid.get_children()
    assert button.get_target_type() == Gtk.Button.__gtype__
    assert not button.is_root()
    assert label.get_target_type() == Gtk.Label.__gtype__
    assert not label.is_root()

    [switch] = flowbox.get_children()
    assert switch.get_target_type() == Gtk.Switch.__gtype__
    assert not switch.is_root()


def test_fragment_parsing_invalid_type():
    with pytest.raises(GLib.GError) as excinfo:
        Grex.Fragment.parse_xml('<GtkThing/>', -1)
    assert 'Unknown type: GtkThing' in excinfo.value.message

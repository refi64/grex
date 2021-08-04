# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GLib, Grex, Gtk
import pytest


def test_empty_element():
    location = Grex.SourceLocation()
    element = Grex.Element.new(Gtk.Box.__gtype__, location)

    assert element.get_root_type() == Gtk.Box.__gtype__
    assert element.get_children() == []
    assert element.get_location() == location


def test_element_children():
    location = Grex.SourceLocation()

    parent, child1, child2 = [
        Grex.Element.new(Gtk.Box.__gtype__, location) for _ in range(3)
    ]

    parent.add_child(child1)
    assert parent.get_children() == [child1]

    parent.add_child(child2)
    assert parent.get_children() == [child1, child2]


def test_element_parsing():
    element = Grex.Element.parse_xml('<GtkBox/>', 'file')
    assert element.get_root_type() == Gtk.Box.__gtype__
    assert element.get_location().get_file() == 'file'


def test_element_parsing_children():
    XML = '''
    <GtkBox>
        <GtkGrid>
            <GtkButton/>
            <GtkLabel/>
        </GtkGrid>

        <GtkFlowBox>
            <GtkSwitch/>
        </GtkFlowBox>
    </GtkBox>
    '''

    root = Grex.Element.parse_xml(XML, None, None)
    assert root.get_root_type() == Gtk.Box.__gtype__

    [grid, flowbox] = root.get_children()
    assert grid.get_root_type() == Gtk.Grid.__gtype__
    assert flowbox.get_root_type() == Gtk.FlowBox.__gtype__

    [button, label] = grid.get_children()
    assert button.get_root_type() == Gtk.Button.__gtype__
    assert label.get_root_type() == Gtk.Label.__gtype__

    [switch] = flowbox.get_children()
    assert switch.get_root_type() == Gtk.Switch.__gtype__


def test_element_parsing_invalid_type():
    with pytest.raises(GLib.GError) as excinfo:
        Grex.Element.parse_xml('<GtkThing/>', None, None)

    assert 'Unknown type: GtkThing' in excinfo.value.message

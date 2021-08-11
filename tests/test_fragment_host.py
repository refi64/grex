# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, Gtk
from unittest.mock import MagicMock


class _MockAttributeDirective(Grex.AttributeDirective):
    def __init__(self):
        super(_MockAttributeDirective, self).__init__()

        self.mock_attach = MagicMock()
        self.mock_update = MagicMock()
        self.mock_detach = MagicMock()

    def do_attach(self, *args):
        self.mock_attach(*args)

    def do_update(self, *args):
        self.mock_update(*args)

    def do_detach(self, *args):
        self.mock_detach(*args)

    def reset_mocks(self):
        self.mock_attach.reset_mock()
        self.mock_update.reset_mock()
        self.mock_detach.reset_mock()


class _AttributeDirectiveX(_MockAttributeDirective):
    pass


class _AttributeDirectiveY(_MockAttributeDirective):
    pass


Grex.DirectiveClass.set_name(_AttributeDirectiveX, 'grex.test-x')
Grex.DirectiveClass.set_name(_AttributeDirectiveY, 'grex.test-y')


def test_fragment_host_construction():
    label = Gtk.Label()
    host = Grex.FragmentHost.new(label)
    assert host.get_applied_properties().get_keys() == []
    assert host.get_widget() == label


def test_fragment_host_matching_type():
    label = Gtk.Label()
    host = Grex.FragmentHost.new(label)

    label_fragment = Grex.Fragment.new(Gtk.Label.__gtype__,
                                       Grex.SourceLocation())
    assert host.matches_fragment_type(label_fragment)

    box_fragment = Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation())
    assert not host.matches_fragment_type(box_fragment)


def test_fragment_host_properties():
    label = Gtk.Label(wrap=False)
    host = Grex.FragmentHost.new(label)

    properties = Grex.PropertySet()
    properties.insert('label', Grex.ValueHolder('hello'))

    host.apply_latest_properties(properties)
    assert label.get_label() == 'hello'
    assert not label.get_wrap()

    properties.insert('wrap', Grex.ValueHolder(True))
    host.apply_latest_properties(properties)
    assert label.get_label() == 'hello'
    assert label.get_wrap()

    properties.remove('label')
    host.apply_latest_properties(properties)
    assert label.get_label() == ''
    assert label.get_wrap()


def test_fragment_inflation_children():
    box = Gtk.Box()
    host = Grex.FragmentHost.new(box)
    host.set_container_adapter(Grex.WidgetContainerAdapter())

    x = Gtk.Label(label='x')
    y = Gtk.Label(label='y')

    host.begin_inflation()
    host.add_inflated_child(0, x)
    assert host.get_leftover_child(0) is None
    host.commit_inflation()

    assert x.get_parent() == box
    assert x.get_prev_sibling() is None
    assert x.get_next_sibling() is None

    host.begin_inflation()
    assert host.get_leftover_child(0) == x
    assert host.get_leftover_child(1) is None
    host.add_inflated_child(0, x)
    assert host.get_leftover_child(0) is None
    assert host.get_leftover_child(1) is None
    host.add_inflated_child(1, y)
    host.commit_inflation()

    assert x.get_parent() == y.get_parent() == box
    assert x.get_prev_sibling() is None
    assert x.get_next_sibling() == y
    assert y.get_next_sibling() is None

    host.begin_inflation()
    assert host.get_leftover_child(0) == x
    assert host.get_leftover_child(1) == y
    host.add_inflated_child(1, y)
    assert host.get_leftover_child(0) == x
    assert host.get_leftover_child(1) is None
    host.add_inflated_child(0, x)
    assert host.get_leftover_child(0) is None
    assert host.get_leftover_child(1) is None
    host.commit_inflation()

    assert x.get_parent() == y.get_parent() == box
    assert x.get_prev_sibling() == y
    assert x.get_next_sibling() is None
    assert y.get_prev_sibling() is None

    host.begin_inflation()
    host.add_inflated_child(1, y)
    host.commit_inflation()

    assert x.get_parent() is None
    assert y.get_parent() == box
    assert y.get_prev_sibling() is None
    assert y.get_next_sibling() is None

    host.begin_inflation()
    host.commit_inflation()

    assert x.get_parent() is None
    assert y.get_parent() is None


def test_fragment_host_inflation_attribute_directives():
    label = Gtk.Label()
    host = Grex.FragmentHost.new(label)

    x = _AttributeDirectiveX()
    y = _AttributeDirectiveY()

    host.begin_inflation()
    host.add_attribute_directive(x)
    host.commit_inflation()

    x.mock_attach.assert_called_once_with(host)
    x.mock_update.assert_called_once_with(host)
    x.mock_detach.assert_not_called()

    x.reset_mocks()

    host.begin_inflation()
    assert host.get_leftover_attribute_directive(_AttributeDirectiveX) == x
    host.add_attribute_directive(x)
    assert host.get_leftover_attribute_directive(_AttributeDirectiveX) is None
    host.add_attribute_directive(y)
    host.commit_inflation()

    x.mock_attach.assert_not_called()
    x.mock_update.assert_called_once_with(host)
    x.mock_detach.assert_not_called()

    y.mock_attach.assert_called_once_with(host)
    y.mock_update.assert_called_once_with(host)
    y.mock_detach.assert_not_called()

    x.reset_mocks()
    y.reset_mocks()

    host.begin_inflation()
    host.add_attribute_directive(y)
    host.commit_inflation()

    x.mock_attach.assert_not_called()
    x.mock_update.assert_not_called
    x.mock_detach.assert_called_once_with(host)

    y.mock_attach.assert_not_called()
    y.mock_update.assert_called_once_with(host)
    y.mock_detach.assert_not_called()

    x.reset_mocks()
    y.reset_mocks()

    host.begin_inflation()
    host.commit_inflation()

    x.mock_attach.assert_not_called()
    x.mock_update.assert_not_called()
    x.mock_detach.assert_not_called()

    y.mock_attach.assert_not_called()
    y.mock_update.assert_not_called()
    y.mock_detach.assert_called_once_with(host)

    x.reset_mocks()
    y.reset_mocks()

    host.begin_inflation()
    host.add_attribute_directive(y)
    host.commit_inflation()

    y.mock_attach.assert_called_once_with(host)
    y.mock_update.assert_called_once_with(host)
    y.mock_detach.assert_not_called()

    y.reset_mocks()

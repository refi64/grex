# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, Gtk
from unittest.mock import MagicMock


class _MockPropertyDirective(Grex.PropertyDirective):
    def __init__(self):
        super(_MockPropertyDirective, self).__init__()

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


class _PropertyDirectiveX(_MockPropertyDirective):
    pass


class _PropertyDirectiveY(_MockPropertyDirective):
    pass


def test_fragment_host_construction():
    label = Gtk.Label()
    host = Grex.FragmentHost.new(label)
    assert host.get_target() == label


def test_fragment_host_matching_type():
    label = Gtk.Label()
    host = Grex.FragmentHost.new(label)

    label_fragment = Grex.Fragment.new(Gtk.Label.__gtype__,
                                       Grex.SourceLocation(), False)
    assert host.matches_fragment_type(label_fragment)

    box_fragment = Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation(),
                                     False)
    assert not host.matches_fragment_type(box_fragment)


def test_fragment_host_properties():
    label = Gtk.Label(wrap=False)
    host = Grex.FragmentHost.new(label)

    label_value = Grex.ValueHolder('hello')
    wrap = Grex.ValueHolder(True)

    host.begin_inflation()
    host.add_property('label', label_value)
    host.commit_inflation()
    assert label.get_label() == 'hello'
    assert not label.get_wrap()

    host.begin_inflation()
    host.add_property('label', label_value)
    host.add_property('wrap', wrap)
    host.commit_inflation()
    assert label.get_label() == 'hello'
    assert label.get_wrap()

    host.begin_inflation()
    host.add_property('wrap', wrap)
    host.commit_inflation()
    assert label.get_label() == ''
    assert label.get_wrap()


def test_fragment_host_signal():
    switch = Gtk.Switch(active=False)
    host = Grex.FragmentHost.new(switch)

    handler = MagicMock()

    host.begin_inflation()
    host.add_signal('notify::active', handler, False)
    host.commit_inflation()

    switch.set_active(True)
    handler.assert_called_once_with(switch, Gtk.Switch.find_property('active'))

    handler.reset_mock()

    host.begin_inflation()
    host.add_property('active', Grex.ValueHolder(False))
    host.add_signal('notify::active', handler, False)
    host.commit_inflation()

    handler.assert_not_called()

    handler.reset_mock()

    host.begin_inflation()
    host.commit_inflation()

    switch.set_active(True)
    handler.assert_not_called()


def test_fragment_inflation_children():
    box = Gtk.Box()
    host = Grex.FragmentHost.new(box)
    host.set_container_adapter(Grex.GtkWidgetContainerAdapter())

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


def test_fragment_host_inflation_property_directives():
    label = Gtk.Label()
    host = Grex.FragmentHost.new(label)

    x = _PropertyDirectiveX()
    y = _PropertyDirectiveY()

    host.begin_inflation()
    host.add_property_directive(0, x)
    host.commit_inflation()

    x.mock_attach.assert_called_once_with(host)
    x.mock_update.assert_called_once_with(host)
    x.mock_detach.assert_not_called()

    x.reset_mocks()

    host.begin_inflation()
    assert host.get_leftover_property_directive(0) == x
    host.add_property_directive(0, x)
    assert host.get_leftover_property_directive(0) is None
    host.add_property_directive(1, y)
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
    host.add_property_directive(1, y)
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
    host.add_property_directive(1, y)
    host.commit_inflation()

    y.mock_attach.assert_called_once_with(host)
    y.mock_update.assert_called_once_with(host)
    y.mock_detach.assert_not_called()

    y.reset_mocks()

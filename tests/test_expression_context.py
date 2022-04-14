# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, GObject
from unittest.mock import MagicMock


# TODO: dedup these across the tests.
class _TestObject(GObject.Object):
    def __init__(self) -> None:
        super(_TestObject, self).__init__()
        self._value = 'abc'

    @GObject.Property(type=str)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, value):
        self._value = value


def test_property_lookup():
    obj = _TestObject()
    context = Grex.ExpressionContext.new(obj)

    dest = GObject.Value()
    success, scope = context.find_name('value', dest)
    assert success
    assert scope is obj
    assert dest.get_string() == 'abc'

    success, scope = context.find_name('missing', dest)
    assert not success
    assert scope is None


def test_name_lookup():
    context = Grex.ExpressionContext.new(None)
    context.insert('name', 123)

    dest = GObject.Value()
    success, scope = context.find_name('name', dest)
    assert success
    assert scope is None
    assert dest.get_int() == 123


def test_name_priority():
    obj = _TestObject()
    context = Grex.ExpressionContext.new(obj)
    context.insert('value', 'def')

    dest = GObject.Value()
    success, scope = context.find_name('value', dest)
    assert success
    assert scope is None
    assert dest.get_string() == 'def'


def test_clone():
    obj = _TestObject()
    context = Grex.ExpressionContext.new(obj)
    context.insert('value', 'def')

    clone = context.clone()
    assert clone.get_scope() is obj

    dest = GObject.Value()
    success, _ = clone.find_name('value', dest)
    assert success
    assert dest.get_string() == 'def'


def test_reset():
    context = Grex.ExpressionContext()
    reset_handler = MagicMock()
    context.connect('reset', reset_handler)

    context.reset_dependencies()
    reset_handler.assert_called_once()

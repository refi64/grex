# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GObject, Grex, Gtk
from unittest.mock import MagicMock


class _TestObject(GObject.Object):
    def __init__(self) -> None:
        super(_TestObject, self).__init__()
        self._value = 10

    @GObject.Property(type=int)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, value):
        self._value = value

    @GObject.Property(type=GObject.Object)
    def inner(self):
        return self._inner


def _build_and_evaluate(
    builder, expected_type, *, context=None, track_dependencies=False
):
    binding = builder.build(Grex.SourceLocation())
    return binding.evaluate(
        expected_type, context or Grex.ExpressionContext(), track_dependencies
    ).get_value()


def test_empty_binding():
    location = Grex.SourceLocation()
    binding = Grex.BindingBuilder().build(location)

    assert binding.get_location() == location
    assert (
        binding.evaluate(str, Grex.ExpressionContext(), False).get_value()
        == ''
    )


def test_constants():
    builder = Grex.BindingBuilder()
    builder.add_constant('ab', -1)
    assert _build_and_evaluate(builder, str) == 'ab'

    builder = Grex.BindingBuilder()
    builder.add_constant('ab', -1)
    builder.add_constant('cd', -1)
    assert _build_and_evaluate(builder, str) == 'abcd'


def test_transform():
    builder = Grex.BindingBuilder()
    builder.add_constant('center', -1)
    assert _build_and_evaluate(builder, Gtk.Align) == Gtk.Align.CENTER


def test_expression():
    obj = _TestObject()
    context = Grex.ExpressionContext.new(obj)

    changed_handler = MagicMock()
    context.connect('changed', changed_handler)

    builder = Grex.BindingBuilder()
    builder.add_expression(
        Grex.property_expression_new(Grex.SourceLocation(), None, 'value'),
        False,
    )
    assert _build_and_evaluate(builder, int, context=context) == 10
    changed_handler.assert_not_called()

    changed_handler.reset_mock()

    builder = Grex.BindingBuilder()
    builder.add_expression(
        Grex.property_expression_new(Grex.SourceLocation(), None, 'value'),
        False,
    )
    assert (
        _build_and_evaluate(
            builder, int, context=context, track_dependencies=True
        )
        == 10
    )
    changed_handler.assert_not_called()

    changed_handler.reset_mock()

    obj.value = 20
    changed_handler.assert_called()


def test_compound():
    obj = _TestObject()
    context = Grex.ExpressionContext.new(obj)

    builder = Grex.BindingBuilder()
    builder.add_constant('abc', -1)
    builder.add_expression(
        Grex.property_expression_new(Grex.SourceLocation(), None, 'value'),
        False,
    )
    assert _build_and_evaluate(builder, str, context=context) == 'abc10'

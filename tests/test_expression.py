# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GLib, GObject, Gtk, Grex
from unittest.mock import MagicMock
import pytest


class _InnerObject(GObject.Object):
    NEXT_VALUE = 'new string'

    def __init__(self) -> None:
        super(_InnerObject, self).__init__()
        self._value = 'string'

    @GObject.Property(type=str)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, value):
        self._value = value

    @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
    def basic_signal(self):
        pass


class _TestObject(GObject.Object):
    NEXT_VALUE = 20

    def __init__(self) -> None:
        super(_TestObject, self).__init__()
        self._value = 10
        self._inner = _InnerObject()

    @GObject.Property(type=int)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, value):
        self._value = value

    @GObject.Property(type=GObject.Object)
    def inner(self):
        return self._inner

    @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
    def basic_signal(self):
        pass

    @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST,
                    arg_types=(int, Gtk.Align),
                    return_type=str)
    def echo_signal(self, x, align):
        return f'args: {x} {align.value_name}'

    @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST
                    | GObject.SignalFlags.DETAILED)
    def detailed_signal(self):
        pass


def _make_property_expr(object, prop):
    return Grex.property_expression_new(Grex.SourceLocation(), object, prop)


def _make_signal_expr(object, signal, *, detail=None, args=[]):
    return Grex.signal_expression_new(Grex.SourceLocation(), object, signal,
                                      detail, args)


def _parse_and_eval(s, context):
    expr = Grex.Expression.parse(s, -1, Grex.SourceLocation())
    return expr.evaluate(context, 0).get_value()


@pytest.fixture
def test_object():
    return _TestObject()


@pytest.fixture(params=[True, False], ids=['inner', 'top-level'])
def target_object(test_object, request):
    return test_object.inner if request.param else test_object


@pytest.fixture
def test_property_expr(target_object):
    is_inner = isinstance(target_object, _InnerObject)
    if is_inner:
        return _make_property_expr(_make_property_expr(None, 'inner'), 'value')
    else:
        return _make_property_expr(None, 'value')


@pytest.fixture
def test_signal_expr(target_object):
    is_inner = isinstance(target_object, _InnerObject)
    if is_inner:
        return _make_signal_expr(_make_property_expr(None, 'inner'),
                                 'basic-signal')
    else:
        return _make_signal_expr(None, 'basic-signal')


@pytest.fixture
def context(test_object):
    return Grex.ExpressionContext.new(test_object)


@pytest.fixture
def changed_handler(context):
    changed_handler = MagicMock()
    context.connect('changed', changed_handler)
    return changed_handler


def test_constant_value_expression(context):
    expr = Grex.constant_value_expression_new(Grex.SourceLocation(), 1)
    assert expr.is_constant()

    result = expr.evaluate(context, Grex.ExpressionEvaluationFlags.NONE)
    assert result.get_value() == 1


def test_property_expression_constant():
    expr = Grex.property_expression_new(Grex.SourceLocation(), None, 'value')
    assert not expr.is_constant()


def test_property_expression_basic(target_object, test_property_expr, context,
                                   changed_handler):
    result = test_property_expr.evaluate(context,
                                         Grex.ExpressionEvaluationFlags.NONE)
    assert not result.can_push()
    assert result.get_value() == target_object.value
    target_object.value = target_object.NEXT_VALUE
    changed_handler.assert_not_called()


def test_property_expression_push(target_object, test_property_expr, context,
                                  changed_handler):
    result = test_property_expr.evaluate(
        context, Grex.ExpressionEvaluationFlags.ENABLE_PUSH)
    assert result.can_push()
    assert result.get_value() == target_object.value
    result.push(target_object.NEXT_VALUE)
    assert target_object.value == target_object.NEXT_VALUE
    changed_handler.assert_not_called()


def test_property_expression_deps(target_object, test_property_expr, context,
                                  changed_handler):
    result = test_property_expr.evaluate(
        context, Grex.ExpressionEvaluationFlags.TRACK_DEPENDENCIES)
    assert not result.can_push()
    assert result.get_value() == target_object.value
    target_object.value = target_object.NEXT_VALUE
    changed_handler.assert_called_once()


def test_property_expression_push_and_deps(target_object, test_property_expr,
                                           context, changed_handler):
    result = test_property_expr.evaluate(
        context, Grex.ExpressionEvaluationFlags.ENABLE_PUSH
        | Grex.ExpressionEvaluationFlags.TRACK_DEPENDENCIES)
    assert result.can_push()
    assert result.get_value() == target_object.value
    result.push(target_object.NEXT_VALUE)
    assert target_object.value == target_object.NEXT_VALUE
    changed_handler.assert_called_once()


def test_property_expression_on_bad_type(context):
    expr = _make_property_expr(_make_property_expr(None, 'value'), 'xyz')
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert excinfo.value.code == Grex.ExpressionEvaluationError.INVALID_TYPE, \
        excinfo.value


def test_property_expression_undefined_global(context):
    expr = _make_property_expr(None, 'xyz')
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code == Grex.ExpressionEvaluationError.UNDEFINED_NAME
            ), excinfo.value


def test_property_expression_undefined_property(context):
    expr = _make_property_expr(_make_property_expr(None, 'inner'), 'xyz')
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code ==
            Grex.ExpressionEvaluationError.UNDEFINED_PROPERTY), excinfo.value


def test_signal_expression_basic(target_object, test_signal_expr, context):
    handler = MagicMock()
    target_object.connect('basic-signal', handler)

    result = test_signal_expr.evaluate(context,
                                       Grex.ExpressionEvaluationFlags.NONE)
    assert result.get_value() is None

    handler.assert_called_once()


def test_signal_expression_args(context):
    expr = _make_signal_expr(
        None,
        'echo-signal',
        args=[
            Grex.constant_value_expression_new(Grex.SourceLocation(), 1),
            Grex.constant_value_expression_new(Grex.SourceLocation(), 'start')
        ])
    result = expr.evaluate(context, Grex.ExpressionEvaluationFlags.NONE)
    assert result.get_value() == 'args: 1 GTK_ALIGN_START'


def test_signal_expression_detail(test_object, context):
    handler = MagicMock()
    test_object.connect('detailed-signal::test', handler)

    expr = _make_signal_expr(None, 'detailed-signal', detail='test')
    expr.evaluate(context, Grex.ExpressionEvaluationFlags.NONE)

    handler.assert_called_once()


def test_signal_expression_undefined_signal(context):
    expr = _make_signal_expr(None, 'xyz')
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code ==
            Grex.ExpressionEvaluationError.UNDEFINED_SIGNAL), excinfo.value


def test_signal_expression_invalid_detail(context):
    expr = _make_signal_expr(None, 'basic-signal', detail='detail')
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code == Grex.ExpressionEvaluationError.INVALID_DETAIL
            ), excinfo.value

    expr = _make_signal_expr(None, 'detailed-signal')
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code == Grex.ExpressionEvaluationError.INVALID_DETAIL
            ), excinfo.value


def test_signal_expression_invalid_args_count(context):
    expr = _make_signal_expr(
        None,
        'basic-signal',
        args=[Grex.constant_value_expression_new(Grex.SourceLocation(), 1)])
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code == Grex.ExpressionEvaluationError.
            INVALID_ARGUMENT_COUNT), excinfo.value


def test_signal_expression_invalid_type(context):
    expr = _make_signal_expr(
        None,
        'echo-signal',
        args=[
            Grex.constant_value_expression_new(Grex.SourceLocation(), 'abc'),
            Grex.constant_value_expression_new(Grex.SourceLocation(), 'def')
        ])
    with pytest.raises(GLib.GError) as excinfo:
        expr.evaluate(context, 0)

    assert (excinfo.value.code == Grex.ExpressionEvaluationError.INVALID_TYPE
            ), excinfo.value


def test_constant_expression_parsing(context):
    assert _parse_and_eval(r"'ab\'c\\\n'", context) == "ab'c\\\n"
    assert _parse_and_eval('12345', context) == 12345
    assert _parse_and_eval('-12345', context) == -12345
    assert _parse_and_eval('0xff', context) == 255
    assert _parse_and_eval('-0xff', context) == -255
    assert _parse_and_eval('true', context) == True
    assert _parse_and_eval('false', context) == False

    with pytest.raises(GLib.GError):
        Grex.Expression.parse("'abc", -1, Grex.SourceLocation())

    with pytest.raises(GLib.GError):
        Grex.Expression.parse('0xy', -1, Grex.SourceLocation())


def test_property_expression_parsing(context):
    assert _parse_and_eval('value', context) == 10
    assert _parse_and_eval('inner.value', context) == 'string'

    with pytest.raises(GLib.GError):
        Grex.Expression.parse('inner.', -1, Grex.SourceLocation())


def test_signal_expression_parsing(test_object, context):
    basic_handler = MagicMock()
    test_object.inner.connect('basic-signal', basic_handler)
    _parse_and_eval('emit inner:basic-signal()', context)
    basic_handler.assert_called_once()

    detailed_handler = MagicMock()
    test_object.connect('detailed-signal::detail', detailed_handler)
    _parse_and_eval('emit detailed-signal::detail()', context)
    detailed_handler.assert_called_once()

    assert _parse_and_eval("emit echo-signal(1, 'end')",
                           context) == 'args: 1 GTK_ALIGN_END'
    assert _parse_and_eval("emit echo-signal(1, 'end', )",
                           context) == 'args: 1 GTK_ALIGN_END'

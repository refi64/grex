# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GObject, Grex, Gtk


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
    return Grex.Fragment.new(Gtk.Label.__gtype__, Grex.SourceLocation())


def _create_box_fragment():
    return Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation())


class _HelloLabelPropertyDirective(Grex.PropertyDirective):
    def do_update(self, host):
        host.get_target().set_text('hello')


class _HelloLabelPropertyDirectiveFactory(Grex.PropertyDirectiveFactory):
    def do_get_name(self):
        return 'test.hello-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.NONE

    def do_create(self):
        return _HelloLabelPropertyDirective()


class _PercentLabelPropertyDirective(Grex.PropertyDirective):
    def __init__(self) -> None:
        super(_PercentLabelPropertyDirective, self).__init__()

        self._value = ''

    @GObject.Property(type=str)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, new_value):
        self._value = new_value

    def do_update(self, host):
        host.add_property('label', Grex.ValueHolder.new(f'{self._value}%'))


class _PercentLabelPropertyDirectiveFactory(Grex.PropertyDirectiveFactory):
    def do_get_name(self):
        return 'test.percent-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.IMPLICIT_VALUE

    def do_create(self):
        return _PercentLabelPropertyDirective()


class _ExplicitPercentLabelPropertyDirective(Grex.PropertyDirective):
    def __init__(self) -> None:
        super(_ExplicitPercentLabelPropertyDirective, self).__init__()

        self._percent = ''
        self._label = ''

    @GObject.Property(type=str)
    def percent(self):  # type: ignore
        return self._percent

    @percent.setter
    def percent(self, new_percent):
        self._percent = new_percent

    @GObject.Property(type=str)
    def label(self):  # type: ignore
        return self._label

    @label.setter
    def label(self, new_label):
        self._label = new_label

    def do_update(self, host):
        host.add_property(
            'label', Grex.ValueHolder.new(f'{self._percent}% {self._label}'))


class _ExplicitPercentLabelPropertyDirectiveFactory(
        Grex.PropertyDirectiveFactory):
    def do_get_name(self):
        return 'test.explicit-percent-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.EXPLICIT

    def do_create(self):
        return _ExplicitPercentLabelPropertyDirective()


class _AutoLabelPropertyDirective(Grex.PropertyDirective):
    def do_update(self, host):
        host.get_target().set_text('auto')


class _AutoLabelPropertyDirectiveFactory(Grex.PropertyDirectiveFactory):
    def do_get_name(self):
        return 'test.auto-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.NONE

    def do_create(self):
        return _AutoLabelPropertyDirective()

    def do_should_auto_attach(self, host, fragment):
        return isinstance(host.get_target(), Gtk.Label)


class _UnlessStructuralDirective(Grex.StructuralDirective):
    def __init__(self) -> None:
        super(_UnlessStructuralDirective, self).__init__()

        self._value = False

    @GObject.Property(type=bool, default=False)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, new_value):
        self._value = new_value

    def do_apply(self, inflator, parent, key, child, flags, child_flags):
        if not self._value:
            inflator.inflate_child(parent, key, child, flags, child_flags)


class _UnlessStructuralDirectiveFactory(Grex.StructuralDirectiveFactory):
    def do_get_name(self):
        return 'test.unless'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.IMPLICIT_VALUE

    def do_create(self):
        return _UnlessStructuralDirective()


def test_inflate_new_target():
    inflator = Grex.Inflator()
    fragment = _create_label_fragment()
    fragment.insert_binding('label', _build_constant_binding('hello'))

    target = inflator.inflate_new_target(fragment, Grex.InflationFlags.NONE)
    assert isinstance(target, Gtk.Label)
    assert target.get_text() == 'hello'


def test_inflate_existing_target():
    inflator = Grex.Inflator()
    fragment = _create_label_fragment()
    fragment.insert_binding('label', _build_constant_binding('hello'))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)
    assert target.get_text() == 'hello'

    fragment.insert_binding('label', _build_constant_binding('world'))
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)
    assert target.get_text() == 'world'


def test_inflate_with_children():
    inflator = Grex.Inflator()
    fragment = _create_box_fragment()
    child_fragment = _create_label_fragment()
    child_fragment.insert_binding('label', _build_constant_binding('hello'))
    fragment.add_child(child_fragment)

    target = Gtk.Box()
    Grex.FragmentHost.new(target).set_container_adapter(
        Grex.WidgetContainerAdapter.new())
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert isinstance(target.get_first_child(), Gtk.Label)
    assert target.get_first_child().get_text() == 'hello'


def test_inflate_with_property_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_HelloLabelPropertyDirectiveFactory()])
    fragment = _create_label_fragment()
    fragment.insert_binding('_test.hello-label', _build_constant_binding(''))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == 'hello'


def test_inflate_with_property_directive_implicit_value():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_PercentLabelPropertyDirectiveFactory()])
    fragment = _create_label_fragment()
    fragment.insert_binding('_test.percent-label',
                            _build_constant_binding('10'))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == '10%'


def test_inflate_with_property_directive_explicit():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_ExplicitPercentLabelPropertyDirectiveFactory()])
    fragment = _create_label_fragment()
    fragment.insert_binding('_test.explicit-percent-label.percent',
                            _build_constant_binding('20'))
    fragment.insert_binding('_test.explicit-percent-label.label',
                            _build_constant_binding('completed'))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == '20% completed'


def test_inflate_with_auto_property_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_AutoLabelPropertyDirectiveFactory()])
    fragment = _create_label_fragment()

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == 'auto'


def test_inflate_with_structural_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_UnlessStructuralDirectiveFactory()])

    fragment = _create_box_fragment()
    child_fragment = _create_label_fragment()
    child_fragment.insert_binding('label', _build_constant_binding('hello'))
    child_fragment.insert_binding('__test.unless', _build_bool_binding(True))
    fragment.add_child(child_fragment)

    target = Gtk.Box()
    Grex.FragmentHost.new(target).set_container_adapter(
        Grex.WidgetContainerAdapter.new())

    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_first_child() is None

    child_fragment.insert_binding('__test.unless', _build_bool_binding(False))
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)
    assert isinstance(target.get_first_child(), Gtk.Label)
    assert target.get_first_child().get_text() == 'hello'

    child_fragment.insert_binding('__test.unless', _build_bool_binding(True))
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)
    assert target.get_first_child() is None

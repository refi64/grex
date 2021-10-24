# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GObject, Grex, Gtk


def _build_constant_binding(value):
    builder = Grex.BindingBuilder.new()
    builder.add_constant(value, -1)
    return builder.build(Grex.SourceLocation())


def _create_label_fragment():
    return Grex.Fragment.new(Gtk.Label.__gtype__, Grex.SourceLocation())


def _create_box_fragment():
    return Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation())


class _HelloLabelAttributeDirective(Grex.AttributeDirective):
    def do_update(self, host):
        host.get_target().set_text('hello')


class _HelloLabelAttributeDirectiveFactory(Grex.DirectiveFactory):
    def do_get_name(self):
        return 'test.hello-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.NONE

    def do_create(self):
        return _HelloLabelAttributeDirective()


class _PercentLabelAttributeDirective(Grex.AttributeDirective):
    def __init__(self) -> None:
        super(_PercentLabelAttributeDirective, self).__init__()

        self._value = ''

    @GObject.Property(type=str)
    def value(self):  # type: ignore
        return self._value

    @value.setter
    def value(self, new_value):
        self._value = new_value

    def do_update(self, host):
        host.add_property('label', Grex.ValueHolder.new(f'{self._value}%'))


class _PercentLabelAttributeDirectiveFactory(Grex.DirectiveFactory):
    def do_get_name(self):
        return 'test.percent-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.IMPLICIT_VALUE

    def do_create(self):
        return _PercentLabelAttributeDirective()


class _ExplicitPercentLabelAttributeDirective(Grex.AttributeDirective):
    def __init__(self) -> None:
        super(_ExplicitPercentLabelAttributeDirective, self).__init__()

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


class _ExplicitPercentLabelAttributeDirectiveFactory(Grex.DirectiveFactory):
    def do_get_name(self):
        return 'test.explicit-percent-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.EXPLICIT

    def do_create(self):
        return _ExplicitPercentLabelAttributeDirective()


class _AutoLabelAttributeDirective(Grex.AttributeDirective):
    def do_update(self, host):
        host.get_target().set_text('auto')


class _AutoLabelAttributeDirectiveFactory(Grex.DirectiveFactory):
    def do_get_name(self):
        return 'test.auto-label'

    def do_get_property_format(self):
        return Grex.DirectivePropertyFormat.NONE

    def do_create(self):
        return _AutoLabelAttributeDirective()

    def do_should_auto_attach(self, host, fragment):
        return isinstance(host.get_target(), Gtk.Label)


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


def test_inflate_with_attribute_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_HelloLabelAttributeDirectiveFactory()])
    fragment = _create_label_fragment()
    fragment.insert_binding('_test.hello-label', _build_constant_binding(''))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == 'hello'


def test_inflate_with_attribute_directive_implicit_value():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_PercentLabelAttributeDirectiveFactory()])
    fragment = _create_label_fragment()
    fragment.insert_binding('_test.percent-label',
                            _build_constant_binding('10'))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == '10%'


def test_inflate_with_attribute_directive_explicit():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_ExplicitPercentLabelAttributeDirectiveFactory()])
    fragment = _create_label_fragment()
    fragment.insert_binding('_test.explicit-percent-label.percent',
                            _build_constant_binding('20'))
    fragment.insert_binding('_test.explicit-percent-label.label',
                            _build_constant_binding('completed'))

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == '20% completed'


def test_inflate_with_auto_attribute_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_AutoLabelAttributeDirectiveFactory()])
    fragment = _create_label_fragment()

    target = Gtk.Label()
    inflator.inflate_existing_target(target, fragment,
                                     Grex.InflationFlags.NONE)

    assert target.get_text() == 'auto'

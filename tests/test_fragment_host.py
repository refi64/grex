from gi.repository import GObject, Grex, Gtk


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
    properties.add('label', GObject.Value(str, 'hello'))

    host.apply_latest_properties(properties)
    assert label.get_label() == 'hello'
    assert not label.get_wrap()

    properties.add('wrap', GObject.Value(bool, True))
    host.apply_latest_properties(properties)
    assert label.get_label() == 'hello'
    assert label.get_wrap()

    properties.remove('label')
    host.apply_latest_properties(properties)
    assert label.get_label() == ''
    assert label.get_wrap()

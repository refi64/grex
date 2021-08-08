from gi.repository import Grex, Gtk


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

    x = Gtk.Label(label='x')
    y = Gtk.Label(label='y')

    host.begin_inflation()
    host.add_inflated_child(0, x)
    assert host.get_leftover_child_from_previous_inflation(0) is None
    host.commit_inflation()

    assert x.get_parent() == box
    assert x.get_prev_sibling() is None
    assert x.get_next_sibling() is None

    host.begin_inflation()
    assert host.get_leftover_child_from_previous_inflation(0) == x
    assert host.get_leftover_child_from_previous_inflation(1) is None
    host.add_inflated_child(0, x)
    assert host.get_leftover_child_from_previous_inflation(0) is None
    assert host.get_leftover_child_from_previous_inflation(1) is None
    host.add_inflated_child(1, y)
    host.commit_inflation()

    assert x.get_parent() == y.get_parent() == box
    assert x.get_prev_sibling() is None
    assert x.get_next_sibling() == y
    assert y.get_next_sibling() is None

    host.begin_inflation()
    assert host.get_leftover_child_from_previous_inflation(0) == x
    assert host.get_leftover_child_from_previous_inflation(1) == y
    host.add_inflated_child(1, y)
    assert host.get_leftover_child_from_previous_inflation(0) == x
    assert host.get_leftover_child_from_previous_inflation(1) is None
    host.add_inflated_child(0, x)
    assert host.get_leftover_child_from_previous_inflation(0) is None
    assert host.get_leftover_child_from_previous_inflation(1) is None
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

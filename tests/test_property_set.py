from gi.repository import GObject, Grex


def test_property_set_basics():
    props = Grex.PropertySet()
    assert not props.contains('test')
    assert props.get_keys() == []

    get_result = GObject.Value()
    assert not props.get('test', get_result)

    value = GObject.Value(int, 10)
    props.insert('test', value)

    assert props.contains('test')
    assert props.get_keys() == ['test']

    get_result = GObject.Value()
    assert props.get('test', get_result)
    assert get_result.get_value() == 10

    value2 = GObject.Value(str, 'abc')
    props.insert('test2', value2)

    assert props.contains('test2')
    # get_keys() is not ordered, so we sort it here.
    assert list(sorted(props.get_keys())) == ['test', 'test2']
    get_result = GObject.Value()
    assert props.get('test2', get_result)
    assert get_result.get_value() == 'abc'

    assert props.remove('test')
    assert props.get_keys() == ['test2']


def _normalize_diff(diff):
    return [list(sorted(names)) for names in diff]


def test_property_set_diff():
    old = Grex.PropertySet()
    new = Grex.PropertySet()

    assert _normalize_diff(old.diff_keys(new)) == [[], [], []]

    test = GObject.Value(int, 10)

    new.insert('test', test)
    assert _normalize_diff(old.diff_keys(new)) == [['test'], [], []]
    assert _normalize_diff(new.diff_keys(old)) == [[], ['test'], []]

    old.insert('test', test)
    assert _normalize_diff(old.diff_keys(new)) == [[], [], ['test']]
    assert _normalize_diff(new.diff_keys(old)) == [[], [], ['test']]

    new.insert('x', test)
    assert _normalize_diff(old.diff_keys(new)) == [['x'], [], ['test']]
    assert _normalize_diff(new.diff_keys(old)) == [[], ['x'], ['test']]

    old.insert('x', test)
    assert _normalize_diff(old.diff_keys(new)) == [[], [], ['test', 'x']]
    assert _normalize_diff(new.diff_keys(old)) == [[], [], ['test', 'x']]

    new.remove('test')
    old.remove('x')
    assert _normalize_diff(old.diff_keys(new)) == [['x'], ['test'], []]
    assert _normalize_diff(new.diff_keys(old)) == [['test'], ['x'], []]

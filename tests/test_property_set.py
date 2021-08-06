from gi.repository import Grex


def test_property_set_basics():
    props = Grex.PropertySet()
    assert not props.contains('test')
    assert props.get_keys() == []
    assert props.get('test') is None

    props.insert('test', Grex.ValueHolder(10))

    assert props.contains('test')
    assert props.get_keys() == ['test']
    assert props.get('test').get_value() == 10

    props.insert('test2', Grex.ValueHolder('abc'))
    assert props.contains('test2')

    # get_keys() is not ordered, so we sort it here.
    assert list(sorted(props.get_keys())) == ['test', 'test2']

    assert props.get('test2').get_value() == 'abc'

    assert props.remove('test')
    assert props.get_keys() == ['test2']


def _normalize_diff(diff):
    return [list(sorted(names)) for names in diff]


def test_property_set_diff():
    old = Grex.PropertySet()
    new = Grex.PropertySet()

    assert _normalize_diff(old.diff_keys(new)) == [[], [], []]

    value = Grex.ValueHolder('value')

    new.insert('test', value)
    assert _normalize_diff(old.diff_keys(new)) == [['test'], [], []]
    assert _normalize_diff(new.diff_keys(old)) == [[], ['test'], []]

    old.insert('test', value)
    assert _normalize_diff(old.diff_keys(new)) == [[], [], ['test']]
    assert _normalize_diff(new.diff_keys(old)) == [[], [], ['test']]

    new.insert('x', value)
    assert _normalize_diff(old.diff_keys(new)) == [['x'], [], ['test']]
    assert _normalize_diff(new.diff_keys(old)) == [[], ['x'], ['test']]

    old.insert('x', value)
    assert _normalize_diff(old.diff_keys(new)) == [[], [], ['test', 'x']]
    assert _normalize_diff(new.diff_keys(old)) == [[], [], ['test', 'x']]

    new.remove('test')
    old.remove('x')
    assert _normalize_diff(old.diff_keys(new)) == [['x'], ['test'], []]
    assert _normalize_diff(new.diff_keys(old)) == [['test'], ['x'], []]

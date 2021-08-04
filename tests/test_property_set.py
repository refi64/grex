from gi.repository import GObject, Grex


def test_property_set_basics():
    props = Grex.PropertySet()
    assert not props.contains('test')
    assert props.get_keys() == []

    get_result = GObject.Value()
    assert not props.get('test', get_result)

    value = GObject.Value(int, 10)
    props.add('test', value)

    assert props.contains('test')
    assert props.get_keys() == ['test']

    get_result = GObject.Value()
    assert props.get('test', get_result)
    assert get_result.get_value() == 10

    value2 = GObject.Value(str, 'abc')
    props.add('test2', value2)

    assert props.contains('test2')
    # get_keys() is not ordered, so we sort it here.
    assert list(sorted(props.get_keys())) == ['test', 'test2']
    get_result = GObject.Value()
    assert props.get('test2', get_result)
    assert get_result.get_value() == 'abc'

    assert props.remove('test')
    assert props.get_keys() == ['test2']

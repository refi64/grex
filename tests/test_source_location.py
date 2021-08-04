# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex


def test_attributes():
    location = Grex.SourceLocation.new('test', 1, 5)
    assert location.get_file() == 'test'
    assert location.get_line() == 1
    assert location.get_column() == 5


def test_missing_file():
    location = Grex.SourceLocation.new(None, 1, 5)
    assert location.get_file() is None


def test_formatting():
    location = Grex.SourceLocation.new('test', 1, 5)
    assert location.format() == 'test:1:5'


def test_formatting_with_missing_file():
    location = Grex.SourceLocation.new(None, 1, 5)
    assert location.format() == '<unknown>:1:5'


def test_formatting_with_zero_line_column():
    zero_line = Grex.SourceLocation.new('file', 0, 5)
    assert zero_line.format() == 'file:?:5'

    zero_column = Grex.SourceLocation.new('file', 1, 0)
    assert zero_column.format() == 'file:1:?'

    zero_line_column = Grex.SourceLocation.new('file', 0, 0)
    assert zero_line_column.format() == 'file:?:?'

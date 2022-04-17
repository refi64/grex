# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree as ET

import gi
import pytest
import subprocess
import uuid

gi.require_version('GLib', '2.0')
gi.require_version('Grex', '1')
gi.require_version('Gtk', '4.0')

TEST_GRESOURCE = 'test.gresource'
TEST_RESOURCE_PREFIX = '/com/refi64/grex/test'
TEST_RESOURCE_CONTENT = 'test-content'


@dataclass
class ResourceDirectory:
    directory: Path
    prefix: str

    @property
    def gresource(self):
        return str(self.directory / TEST_GRESOURCE)

    @property
    def content_path(self):
        return f'{self.prefix}/{TEST_RESOURCE_CONTENT}'

    def compile_content(self, content):
        resource_xml_path = self.directory / f'{TEST_GRESOURCE}.xml'
        content_path = self.directory / TEST_RESOURCE_CONTENT

        with content_path.open('w') as fp:
            fp.write(content)

        root = ET.Element('gresources')
        resource = ET.SubElement(root, 'gresource', {'prefix': self.prefix})
        ET.SubElement(resource, 'file').text = content_path.name

        ET.ElementTree(root).write(resource_xml_path)

        subprocess.run(['glib-compile-resources', resource_xml_path.name],
                       cwd=self.directory,
                       check=True)


@pytest.fixture
def resource_directory(tmp_path):
    return ResourceDirectory(tmp_path,
                             f'{TEST_RESOURCE_PREFIX}/{uuid.uuid4().hex}')

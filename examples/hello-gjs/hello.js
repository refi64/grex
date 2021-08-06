import gi from 'gi'

import Gio from 'gi://Gio'
import GIRepository from 'gi://GIRepository'
import GObject from 'gi://GObject'
import Gtk from 'gi://Gtk?version=4.0'

// This isn't really specific to Grex at all, it's just some tricks to load our
// resources from the build directory & locate Grex.
function setupEnvironment() {
  let scriptDir = (() => {
    let stack = (new Error()).stack.trim().split('\n')
    let scriptEntry = stack[stack.length - 1]
    let match = scriptEntry.match(/^@file:\/\/([^:]+)/)
    let scriptFile = Gio.File.new_for_path(match[1])
    return scriptFile.get_parent()
  })()

  let buildRoot = scriptDir
  while (true) {
    if (buildRoot.get_child('build.ninja').query_exists(null)) {
      break
    }

    buildRoot = buildRoot.get_parent()
    if (!buildRoot) {
      throw new Error("Couldn't find root build directory")
    }
  }

  let compiledOutputsDir = buildRoot.get_child('src')
  let helloResourcesFile = buildRoot
    .get_child('examples')
    .get_child('hello-gjs')
    .get_child('hello-resources.gresource')

  GIRepository.Repository.prepend_search_path(compiledOutputsDir.get_path())
  GIRepository.Repository.prepend_library_path(compiledOutputsDir.get_path())

  let helloResource = Gio.Resource.load(helloResourcesFile.get_path())
  helloResource._register()
}

setupEnvironment()
const Grex = gi.require('Grex')

// Now we get onto the Grex-related stuff!

const HelloWindow = GObject.registerClass({
  GTypeName: 'HelloWindow',
}, class HelloWindow extends Gtk.ApplicationWindow {
  static template = Grex.Template.new_from_resource(
      '/org/hello/Hello/hello-window.xml', null)

  _init(application) {
    super._init({ application })

    this.inflator = new Grex.Inflator()
    HelloWindow.template.inflate(this.inflator, this)
  }
})

function main(argv) {
  setupEnvironment()

  const app = new Gtk.Application({
    application_id: 'org.hello.Hello',
    flags: Gio.ApplicationFlags.FLAGS_NONE,
  })

  app.connect('activate', (application) => {
    let window = new HelloWindow(app)
    window.present()
  })

  return app.run(argv)
}

imports.package.run({ main })

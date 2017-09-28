const { script, command, file } = require("../cli")

script((argv) => {
  console.log(`package config of '${argv.source}' to '${argv.destination}'...`)

  const package_json = file.read.json(`${argv.source}/package.json`)
  package_json.name = argv.name || package_json.name
  package_json.version = argv.version
  if (argv.production) {
    package_json.devDependencies = undefined
    package_json.scripts = undefined
    package_json.private = undefined
  }
  else {
    package_json.private = true
  }
  file.write.json(`${argv.destination}/package.json`, package_json)

  const haveDependencies = package_json.dependencies || package_json.devDependencies || package_json.peerDependencies
  if ("install" in argv && haveDependencies) {
    console.log(`> package install...`)
    command.exec("npm install", argv.destination)
  }
})

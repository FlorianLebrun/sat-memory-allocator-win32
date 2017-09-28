const { script, command, file } = require("../cli")

script((argv) => {
  const isRelease = (argv.debug != 1)

  // Create package
  console.log(`npm development pack of '${argv.name}'...`)
  const dev_version = `${argv.version}-${isRelease ? "release" : "debug"}`
  file.write.json("./package.json", {
    name: argv.name,
    version: dev_version,
  })
  file.write.text("./.npmignore", [
    "/*.log",
    "/*.tgz",
  ])
  command.exec("npm pack")
  file.move.toDir(`${argv.name}-${dev_version}.tgz`, argv.output)

  // Create light package for release
  if (isRelease) {
    console.log(`npm production pack of '${argv.name}'...`)
    file.write.json("./package.json", {
      name: argv.name,
      version: argv.version,
    })
    file.write.text("./.npmignore", [
      "/*.log",
      "/*.tgz",
      "*.pdb",
    ])
    command.exec("npm pack")
    file.move.toDir(`${argv.name}-${argv.version}.tgz`, argv.output)
  }
})

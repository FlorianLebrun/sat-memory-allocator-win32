const { script, command, file } = require("../cli")

script((argv) => {
  if (file.exists(argv.input)) {
    console.log(`copy of '${argv.input}' to '${argv.output || argv.destination}'...`)
    if (argv.output) file.copy.toFile(argv.input, argv.output)
    else if (argv.destination) file.copy.toDir(argv.input, argv.destination)
    else command.exit(-1)
  }
})

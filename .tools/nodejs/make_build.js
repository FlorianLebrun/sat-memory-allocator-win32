const { script, command, file } = require("../cli")
const fs = require("fs")

script((argv) => {
  console.log(`build of '${argv.source}' to '${argv.destination}'...`)
  if(!fs.existsSync(`${argv.source}/node_modules`)) {
    command.exec(`npm install`, argv.source)
  }
  command.exec(`npm run build`, argv.source)
})

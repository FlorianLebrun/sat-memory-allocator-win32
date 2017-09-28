const { script, command, file } = require("../cli")
const Path = require("path")
const fs = require("fs")

script((argv) => {
  console.log(`copy and sign of '${argv.input}' to '${argv.output}'...`)

  // Copy file
  let outputFile
  if (argv.output) outputFile = file.copy.toFile(argv.input, argv.output)
  else if (argv.destination) outputFile = file.copy.toDir(argv.input, argv.destination)
  else command.exit(-1)

  // Sign file with certificate
  const certificate_path = Path.resolve(__dirname, "../certificate/build-certificate.pfx")
  const certificate_password = "my_password"
  command.exec(`"${__dirname}/signtool.exe" sign /f "${certificate_path}" /p "${certificate_password}" /fd SHA1 /tr http://timestamp.digicert.com /td SHA1 /a ${outputFile}`)
  command.exec(`"${__dirname}/signtool.exe" sign /f "${certificate_path}" /p "${certificate_password}" /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /as ${outputFile}`)
})

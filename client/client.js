import net from 'net'
import readline from 'readline'

const client = net.createConnection({ port: 8000 }, () => {
  console.log('connected to server')
})

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
})

rl.on('line', (line) => {
  // 🔥 Split multiple commands using ;
  const commands = line.split(';')

  let finalPayload = ''

  for (const cmd of commands) {
    const parts = cmd.trim().split(' ')
    if (parts[0] === '') continue

    let resp = `*${parts.length}\r\n`
    for (const part of parts) {
      resp += `$${part.length}\r\n${part}\r\n`
    }

    finalPayload += resp
  }

  // 🔥 send ALL commands in one go
  client.write(finalPayload)
})

client.on('data', (data) => {
  console.log('response:', data.toString())
})

client.on('end', () => {
  console.log('disconnected from server')
})

client.on('error', (err) => {
  console.log('client error:', err.message)
})
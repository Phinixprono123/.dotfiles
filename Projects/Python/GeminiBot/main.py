#imports 
import json
import os
import subprocess

#Setting variables
API_KEY = "AIzaSyB6h5k3UOa0ZIOdQT4i35FPPDn_DwZzw_k"
HISTORY_FILE = "ChatHist.json"

def speak(output):
    output = output.replace("*", " ")
    output = output.replace(":", ",")
    output = output.replace("#", " ")
    subprocess.run(['espeak', '-v', 'en-us+m3', '-p', '60', output], text=True)



# Loads the conversation history from HIST_FILE 
def load_history():
    # Creates the file in HISTORY_FILE if it dosen't exists
    if not os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, "w") as file:
            json.dump([], file)
    with open(HISTORY_FILE, "r") as file:
        return json.load(file)
# saves the history to HISTORY_FILE uhhh im tired of typing bye
def save_history(history):
    with open(HISTORY_FILE, "w") as file:
        json.dump(history, file, indent=2)

#Function to interact with Gemini API and get response
def generate_text(promt, maxtokens=50):
    history = load_history()
    history.append({"text": promt})

    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key={API_KEY}"
    data = {
        "contents": [{
            "parts": history + [{"text": promt}]
        }],
        "generationConfig": {
            "maxOutputTokens": maxtokens
        }
    }
    command = ['curl', url, "-H", "Content-Type: application/json", "-X", "POST", "-d", json.dumps(data)]
    result = subprocess.run(command, capture_output=True, text=True)
    return json.loads(result.stdout)

# Main Function to setup chating system
 
def main():
    print("Gemini 2.0 Flash running")
    history = load_history()
    while True:
        userinput = input("You: ")
        if userinput == "/exit" or userinput == "/stop":
            break

        history.append({"text": f"You: {userinput}"})

        response = generate_text(userinput, 200)
        if "candidates" in response:
            ai_output = response["candidates"][0]["content"]["parts"][0]["text"]
            history.append({"text": f"Aissistant: {ai_output}"})
            save_history(history)
            print(ai_output)
            speak(ai_output)
        else:
            print(f"Unexpected response: {response}")

if __name__ == "__main__":
    main()

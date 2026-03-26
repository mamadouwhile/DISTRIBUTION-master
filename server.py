from flask import Flask, render_template, Response, stream_with_context
import subprocess
import html

app = Flask(__name__)

@app.route("/")
def home():
    return render_template("index.html")

def stream_process(mode):
    cmd = ["./Arene_light"]

    if mode == "random-mcts":
        cmd.append("random-mcts")
        duel_name = "Random vs MCTS"
    elif mode == "mcts-brutal":
        cmd.append("mcts-brutal")
        duel_name = "MCTS vs Brutal"
    else:
        yield "event: error\ndata: Mode inconnu\n\n"
        return

    yield f"event: start\ndata: {duel_name}\n\n"

    try:
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        for line in process.stdout:
            safe_line = html.escape(line.rstrip())
            yield f"event: line\ndata: {safe_line}\n\n"

        process.wait()

        if process.returncode == 0:
            yield "event: end\ndata: Compétition terminée\n\n"
        else:
            yield f"event: error\ndata: Le processus s'est terminé avec le code {process.returncode}\n\n"

    except Exception as e:
        yield f"event: error\ndata: {html.escape(str(e))}\n\n"

@app.route("/stream/<mode>")
def stream(mode):
    return Response(
        stream_with_context(stream_process(mode)),
        mimetype="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "X-Accel-Buffering": "no"
        }
    )

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=10000, debug=True, threaded=True)

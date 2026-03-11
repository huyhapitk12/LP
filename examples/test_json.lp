import json

def test_json_roundtrip():
    payload = json.loads('{"project":"LP","build":{"id":7,"ok":true},"tags":["docs","examples"]}')
    if str(payload["project"]) != "LP":
        raise "json project wrong"

    build = payload["build"]
    if int(build["id"]) != 7:
        raise "json nested id wrong"
    if not bool(build["ok"]):
        raise "json nested bool wrong"

    tags = payload["tags"]
    if str(tags[1]) != "examples":
        raise "json list access wrong"

    dumped = json.dumps(payload)
    if dumped.find('"project": "LP"') < 0:
        raise "json dumps missing project"

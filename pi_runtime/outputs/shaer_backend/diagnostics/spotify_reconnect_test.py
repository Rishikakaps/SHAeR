#!/usr/bin/env python3
from shaer_music import SpotifyServices


class Auth:
    def status(self):
        return {"authenticated": True}

    def access_token(self):
        return "redacted"


class Client:
    def playback_state(self):
        return {"is_playing": False}


class Connect:
    started = False

    def installed(self):
        return True

    def service_active(self):
        return False

    def start(self):
        self.started = True
        return True


def main():
    connect = Connect()
    SpotifyServices(Auth(), Client(), connect).tick()
    assert connect.started
    print("spotify_reconnect_test ok service_restart=true")


if __name__ == "__main__":
    main()

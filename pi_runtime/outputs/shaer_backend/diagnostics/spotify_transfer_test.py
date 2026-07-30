#!/usr/bin/env python3
from shaer_music import LibrespotManager


class Client:
    def __init__(self):
        self.transferred = None

    def devices(self):
        return {"devices": [{"id": "shaer-device", "name": "SHAeR"}]}

    def transfer(self, device_id, play=False):
        self.transferred = (device_id, play)


def main():
    client = Client()
    LibrespotManager(client).transfer(play=True)
    assert client.transferred == ("shaer-device", True)
    print("spotify_transfer_test ok target=SHAeR play=true")


if __name__ == "__main__":
    main()

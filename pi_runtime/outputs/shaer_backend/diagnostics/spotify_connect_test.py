#!/usr/bin/env python3
from shaer_music import LibrespotManager


class Client:
    def devices(self):
        return {"devices": [{"id": "shaer", "name": "SHAeR"}]}


class Result:
    returncode = 0


def main():
    manager = LibrespotManager(Client(), runner=lambda *_args, **_kwargs: Result())
    status = manager.status()
    assert status.service_active and status.discovered and status.device_id == "shaer"
    print(f"spotify_connect_test ok discovered={status.discovered} binary_present={status.installed}")


if __name__ == "__main__":
    main()

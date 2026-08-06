"""librespot lifecycle and Spotify Connect device helpers."""

from __future__ import annotations

import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from .spotify_client import SpotifyClient


@dataclass(frozen=True, slots=True)
class ConnectStatus:
    installed: bool
    service_active: bool
    discovered: bool
    device_id: str | None
    device_name: str = "SHAeR"

    def public_dict(self) -> dict[str, object]:
        return {
            "installed": self.installed,
            "service_active": self.service_active,
            "discovered": self.discovered,
            "device_id": self.device_id,
            "device_name": self.device_name,
        }


Runner = Callable[..., subprocess.CompletedProcess[str]]


class LibrespotManager:
    def __init__(
        self,
        client: SpotifyClient | None = None,
        service_name: str = "shaer-librespot.service",
        device_name: str = "SHAeR",
        runner: Runner = subprocess.run,
    ):
        self.client = client
        self.service_name = service_name
        self.device_name = device_name
        self._runner = runner

    def installed(self) -> bool:
        return shutil.which("librespot") is not None

    def service_active(self) -> bool:
        try:
            result = self._runner(
                ["systemctl", "is-active", "--quiet", self.service_name],
                capture_output=True,
                text=True,
                check=False,
            )
        except FileNotFoundError:
            return False
        return result.returncode == 0

    def start(self) -> bool:
        try:
            result = self._runner(["systemctl", "start", self.service_name], capture_output=True, text=True, check=False)
        except FileNotFoundError:
            return False
        return result.returncode == 0

    def stop(self) -> bool:
        try:
            result = self._runner(["systemctl", "stop", self.service_name], capture_output=True, text=True, check=False)
        except FileNotFoundError:
            return False
        return result.returncode == 0

    def restart(self) -> bool:
        try:
            result = self._runner(["systemctl", "restart", self.service_name], capture_output=True, text=True, check=False)
        except FileNotFoundError:
            return False
        return result.returncode == 0

    def shaer_device(self) -> dict[str, object] | None:
        if self.client is None:
            return None
        payload = self.client.devices()
        devices = payload.get("devices", []) if isinstance(payload, dict) else []
        for device in devices:
            if isinstance(device, dict) and device.get("name") == self.device_name:
                return device
        return None

    def status(self) -> ConnectStatus:
        device = self.shaer_device()
        return ConnectStatus(
            installed=self.installed(),
            service_active=self.service_active(),
            discovered=device is not None,
            device_id=str(device.get("id")) if device and device.get("id") else None,
            device_name=self.device_name,
        )

    def transfer(self, play: bool = False) -> None:
        if self.client is None:
            raise RuntimeError("Spotify Web API is not configured.")
        device = self.shaer_device()
        if not device or not device.get("id"):
            raise RuntimeError("SHAeR is not currently visible as a Spotify Connect device.")
        self.client.transfer(str(device["id"]), play=play)

    @staticmethod
    def service_unit(
        librespot_path: str = "/usr/local/bin/librespot",
        runtime_user: str = "shaer",
        config_dir: str = "/etc/shaer",
        cache_dir: str = "/var/cache/shaer",
    ) -> str:
        return f"""[Unit]
Description=SHAeR Spotify Connect receiver
After=network-online.target sound.target
Wants=network-online.target

[Service]
Type=simple
User={runtime_user}
EnvironmentFile=-{config_dir}/spotify.env
ExecStart={librespot_path} --name SHAeR --backend alsa --device ${{SHAER_ALSA_DEVICE}} --cache {cache_dir}/librespot --bitrate 320
Restart=always
RestartSec=3
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
"""

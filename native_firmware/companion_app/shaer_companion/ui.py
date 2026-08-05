from __future__ import annotations

import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from . import APP_NAME
from .app_core import CompanionAppCore


class CompanionWindow(tk.Tk):
    def __init__(self, core: CompanionAppCore) -> None:
        super().__init__()
        self.core = core
        self.status_queue: queue.Queue[str] = queue.Queue()
        self.title(APP_NAME)
        self.geometry("1120x720")
        self.minsize(920, 560)
        self.configure(bg="#10131a")
        self._build_style()
        self._build()
        self.core.events.subscribe("*", self._on_event)
        self.after(150, self._pump_events)
        self.after(150, self._pump_status)
        self.refresh_library()

    def _build_style(self) -> None:
        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        style.configure("TNotebook", background="#10131a", borderwidth=0)
        style.configure("TNotebook.Tab", padding=(18, 10))
        style.configure("Treeview", rowheight=26)
        style.configure("Status.TLabel", background="#10131a", foreground="#d9d7ff")

    def _build(self) -> None:
        header = ttk.Frame(self, padding=(16, 12))
        header.pack(fill="x")
        ttk.Label(header, text="SHAeR Companion", font=("Helvetica", 22, "bold")).pack(side="left")
        ttk.Button(header, text="Import Music", command=self.import_music).pack(side="right", padx=4)
        ttk.Button(header, text="Sync SD Card", command=self.sync_device).pack(side="right", padx=4)

        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill="both", expand=True, padx=12, pady=8)

        self.library_tree = self._library_tab()
        self.sync_text = self._device_tab()
        self.theme_list = self._themes_tab()
        self.log_list = self._logs_tab()
        self.settings_text = self._settings_tab()

        self.status = ttk.Label(self, text="Ready", style="Status.TLabel", anchor="w", padding=(12, 8))
        self.status.pack(fill="x")

    def _library_tab(self) -> ttk.Treeview:
        frame = ttk.Frame(self.notebook, padding=8)
        self.notebook.add(frame, text="Library")
        columns = ("title", "artist", "album", "format", "year")
        tree = ttk.Treeview(frame, columns=columns, show="headings")
        for column in columns:
            tree.heading(column, text=column.title())
            tree.column(column, width=180 if column in {"title", "album"} else 120)
        tree.pack(fill="both", expand=True)
        return tree

    def _device_tab(self) -> tk.Text:
        frame = ttk.Frame(self.notebook, padding=8)
        self.notebook.add(frame, text="Device")
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill="x")
        ttk.Button(toolbar, text="Inspect SD Card", command=self.inspect_device).pack(side="left", padx=4)
        ttk.Button(toolbar, text="Preview Sync", command=self.preview_sync).pack(side="left", padx=4)
        text = tk.Text(frame, height=18, wrap="word")
        text.pack(fill="both", expand=True, pady=8)
        return text

    def _themes_tab(self) -> tk.Listbox:
        frame = ttk.Frame(self.notebook, padding=8)
        self.notebook.add(frame, text="Themes")
        ttk.Button(frame, text="Install Theme Folder", command=self.install_theme).pack(anchor="w", pady=4)
        listing = tk.Listbox(frame)
        listing.pack(fill="both", expand=True)
        self.refresh_themes(listing)
        return listing

    def _logs_tab(self) -> tk.Listbox:
        frame = ttk.Frame(self.notebook, padding=8)
        self.notebook.add(frame, text="Logs")
        listing = tk.Listbox(frame)
        listing.pack(fill="both", expand=True)
        return listing

    def _settings_tab(self) -> tk.Text:
        frame = ttk.Frame(self.notebook, padding=8)
        self.notebook.add(frame, text="Settings")
        controls = ttk.Frame(frame)
        controls.pack(fill="x")
        ttk.Button(controls, text="Backup Settings", command=self.backup_settings).pack(side="left", padx=4)
        ttk.Button(controls, text="Restore Settings", command=self.restore_settings).pack(side="left", padx=4)
        text = tk.Text(frame, height=18, wrap="word")
        text.insert("end", "Bluetooth, Wi-Fi, power, audio, display, recording and device settings are stored in SQLite.\n")
        text.pack(fill="both", expand=True, pady=8)
        return text

    def import_music(self) -> None:
        folder = filedialog.askdirectory(title="Choose music folder")
        if not folder:
            return
        self._background(lambda: self.core.library.import_folder(Path(folder)), "Importing music...")

    def sync_device(self) -> None:
        folder = filedialog.askdirectory(title="Choose SHAeR SD card root")
        if not folder:
            return
        self._background(lambda: self.core.sync.execute(Path(folder)), "Synchronizing SD card...")

    def preview_sync(self) -> None:
        folder = filedialog.askdirectory(title="Choose SHAeR SD card root")
        if not folder:
            return
        plan = self.core.sync.plan(Path(folder))
        self.sync_text.delete("1.0", "end")
        self.sync_text.insert("end", f"Files to copy: {len(plan.items)}\n")
        self.sync_text.insert("end", f"Bytes: {plan.total_bytes}\n")
        self.sync_text.insert("end", f"Already current: {plan.skipped_duplicates}\n\n")
        for item in plan.items[:200]:
            self.sync_text.insert("end", f"{item.source_path} -> {item.destination_path}\n")

    def inspect_device(self) -> None:
        folder = filedialog.askdirectory(title="Choose SHAeR SD card root")
        if not folder:
            return
        info = self.core.device.inspect(Path(folder))
        self.sync_text.delete("1.0", "end")
        self.sync_text.insert("end", f"Device path: {info.path}\n")
        self.sync_text.insert("end", f"Firmware: {info.firmware_version}\n")
        self.sync_text.insert("end", f"Free: {info.free_bytes / (1024 ** 3):.2f} GB\n")
        self.sync_text.insert("end", f"Total: {info.total_bytes / (1024 ** 3):.2f} GB\n")

    def install_theme(self) -> None:
        folder = filedialog.askdirectory(title="Choose theme folder")
        if not folder:
            return
        try:
            self.core.theme.install_theme(Path(folder))
            self.refresh_themes(self.theme_list)
        except Exception as exc:
            messagebox.showerror("Theme install failed", str(exc))

    def backup_settings(self) -> None:
        path = filedialog.asksaveasfilename(title="Backup settings", defaultextension=".json")
        if path:
            self.core.settings.backup(Path(path))

    def restore_settings(self) -> None:
        path = filedialog.askopenfilename(title="Restore settings", filetypes=[("JSON", "*.json"), ("All", "*")])
        if path:
            self.core.settings.restore(Path(path))

    def refresh_library(self) -> None:
        for item in self.library_tree.get_children():
            self.library_tree.delete(item)
        for track in self.core.library.tracks():
            self.library_tree.insert("", "end", values=(track["title"], track["artist"], track["album"], track["file_format"], track["year"]))

    def refresh_themes(self, listing: tk.Listbox) -> None:
        listing.delete(0, "end")
        for theme in self.core.theme.themes():
            listing.insert("end", f"{theme.get('display_name', theme.get('id', 'Unknown'))}")

    def _background(self, fn, status: str) -> None:
        self.status.configure(text=status)

        def run() -> None:
            try:
                result = fn()
                self.status_queue.put(f"Done: {result}")
            except Exception as exc:
                self.status_queue.put(f"Error: {exc}")

        threading.Thread(target=run, daemon=True).start()

    def _on_event(self, event) -> None:
        self.log_list.insert("end", f"{event.name}  {event.payload}")
        self.log_list.yview_moveto(1.0)

    def _pump_events(self) -> None:
        self.core.events.drain()
        self.after(150, self._pump_events)

    def _pump_status(self) -> None:
        try:
            while True:
                message = self.status_queue.get_nowait()
                self.status.configure(text=message)
                self.refresh_library()
        except queue.Empty:
            pass
        self.after(200, self._pump_status)


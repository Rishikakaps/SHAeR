export const themes = [
  {
    id: "archive_dark",
    label: "Archive Dark",
    tone: "Recovered terminal object",
    palette: {
      bg: "#000000",
      panel: "#c0c0c0",
      ink: "#ffffff",
      muted: "#8c8c8c",
      dim: "#3c3c3c",
      accent: "#e63950",
      accent2: "#4a90d9",
      select: "#000080",
      selectInk: "#ffffff",
      line: "#ffffff"
    },
    typography: "mono",
    chrome: "win95_dark",
    motion: "scanline",
    menu: ["SPOTIFY CONNECT", "LOCAL FILES", "VOICE MEMOS", "SETTINGS"],
    library: ["01 CLOUD_SONGS", "02 PLAYLIST 1", "03 FLAC_TEST", "04 VOICE_12"],
    settings: ["APPEARANCE", "AUDIO", "PLAYBACK", "CONNECTIVITY", "STORAGE", "POWER", "ADVANCED", "ABOUT"]
  },
  {
    id: "bombay_ticket",
    label: "Bombay Ticket",
    tone: "Local train ticket stub",
    palette: {
      bg: "#f4e9ca",
      panel: "#fff8df",
      ink: "#2f241a",
      muted: "#765235",
      dim: "#b58b58",
      accent: "#b9342d",
      accent2: "#1f6655",
      select: "#b9342d",
      selectInk: "#fff8df",
      line: "#3a2a1e"
    },
    typography: "ticket",
    chrome: "paper",
    motion: "punch",
    menu: ["Menu Bar", "Spotify Connect", "Local Files", "Settings"],
    library: ["LIKED SONGS", "PLAYLIST 1", "TRAIN MIX", "VOICE NOTE"],
    settings: ["Appearance", "Audio", "Playback", "Storage", "Power", "About"]
  },
  {
    id: "indian_raga",
    label: "Indian Raga",
    tone: "Indigo mehfil instrument",
    palette: {
      bg: "#030126",
      panel: "#11073a",
      ink: "#ded4ff",
      muted: "#9d8ace",
      dim: "#55476f",
      accent: "#8e3417",
      accent2: "#c9a7ff",
      select: "#8e3417",
      selectInk: "#fff1df",
      line: "#ded4ff"
    },
    typography: "raga",
    chrome: "ornament",
    motion: "curtain",
    menu: ["NOW PLAYING", "LOCAL RAAG", "VOICE ALAAP", "SETTINGS"],
    library: ["RAGA I - CLOUD SONG", "BANDISH 02", "VOICE MEMORY", "TANPURA LOOP"],
    settings: ["Appearance", "Sur", "Playback", "Storage", "Power", "About"]
  },
  {
    id: "windows_xp",
    label: "Windows XP",
    tone: "Tiny desktop appliance",
    palette: {
      bg: "#ece9d8",
      panel: "#ffffff",
      ink: "#101010",
      muted: "#4b4b4b",
      dim: "#8d897d",
      accent: "#245edb",
      accent2: "#3ba83b",
      select: "#316ac5",
      selectInk: "#ffffff",
      line: "#7f9db9"
    },
    typography: "xp",
    chrome: "xp",
    motion: "window",
    menu: ["Now Playing.exe", "My Music", "Voice Archive", "Control Panel"],
    library: ["Cloud Song.mp3", "Liked Songs", "Playlist 1", "Voice Memo.wav"],
    settings: ["Appearance", "Audio", "Playback", "Network", "Power", "System"]
  },
  {
    id: "japanese_punk",
    label: "Japanese Punk",
    tone: "Neon zine electronics",
    palette: {
      bg: "#111111",
      panel: "#1d1d1d",
      ink: "#f4efe8",
      muted: "#8e8e8e",
      dim: "#4c4c4c",
      accent: "#f20a66",
      accent2: "#70f1d1",
      select: "#f20a66",
      selectInk: "#ffffff",
      line: "#f4efe8"
    },
    typography: "punk",
    chrome: "zine",
    motion: "hardcut",
    menu: ["TRACK 07", "LOCAL FILES", "VOICE TAPE", "SETUP"],
    library: ["CLOUD SONG", "LIKED NOISE", "PUNCH MIX", "MEMO TAKE 01"],
    settings: ["APPEARANCE", "AUDIO", "PLAYBACK", "STORAGE", "POWER", "ABOUT"]
  },
  {
    id: "ghibli_garden",
    label: "Ghibli Garden",
    tone: "Quiet handmade garden",
    palette: {
      bg: "#e8dfc8",
      panel: "#f8f1dc",
      ink: "#44382e",
      muted: "#746b58",
      dim: "#a99d7c",
      accent: "#d6906e",
      accent2: "#86a97c",
      select: "#86a97c",
      selectInk: "#fffdf2",
      line: "#746b58"
    },
    typography: "soft",
    chrome: "watercolor",
    motion: "leaf",
    menu: ["Now Playing", "Local Shelf", "Voice Jar", "Settings"],
    library: ["Cloud Song", "Rain Playlist", "Garden Walk", "Voice Memory"],
    settings: ["Appearance", "Audio", "Playback", "Storage", "Power", "About"]
  }
];

export const screens = [
  { id: "boot", label: "Boot" },
  { id: "home", label: "Home" },
  { id: "library", label: "Library" },
  { id: "now", label: "Now Playing" },
  { id: "settings", label: "Settings" },
  { id: "spotify_drop", label: "Spotify Drop" },
  { id: "charging", label: "Charging" },
  { id: "sleep", label: "Sleep" }
];

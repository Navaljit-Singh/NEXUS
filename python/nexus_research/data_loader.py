"""Data loader for NEXUS Parquet tick and bar files written by the C++ engine."""

import pathlib
from typing import List

import pyarrow.parquet as pq
import polars as pl
import pandas as pd


class ParquetDataLoader:
    """Loads NEXUS tick/bar Parquet files written by the C++ engine."""

    def __init__(self, data_dir: str) -> None:
        self.data_dir = pathlib.Path(data_dir)

    def load_ticks(self, instrument_id: int, date_str: str) -> pl.DataFrame:
        """Load ticks for a given instrument and date (YYYY-MM-DD).

        Returns a polars DataFrame with tick data.

        Args:
            instrument_id: Numeric instrument identifier used by the engine.
            date_str: Date string in YYYY-MM-DD format.

        Raises:
            FileNotFoundError: If the Parquet file does not exist.
        """
        path = self.data_dir / f"ticks_{instrument_id}_{date_str}.parquet"
        if not path.exists():
            raise FileNotFoundError(f"Tick file not found: {path}")
        return pl.read_parquet(path)

    def load_bars(self, instrument_id: int, interval_seconds: int, date_str: str) -> pl.DataFrame:
        """Load OHLCV bars for a given instrument, resolution, and date.

        Args:
            instrument_id: Numeric instrument identifier.
            interval_seconds: Bar interval in seconds (e.g. 60 for 1-minute bars).
            date_str: Date string in YYYY-MM-DD format.

        Raises:
            FileNotFoundError: If the Parquet file does not exist.
        """
        path = self.data_dir / f"bars_{instrument_id}_{interval_seconds}s_{date_str}.parquet"
        if not path.exists():
            raise FileNotFoundError(f"Bar file not found: {path}")
        return pl.read_parquet(path)

    def list_available(self) -> List[str]:
        """Return names of all Parquet files found in the data directory."""
        return [p.name for p in self.data_dir.glob("*.parquet")]

"""Event-driven backtesting engine for NEXUS strategies using polars DataFrames."""

from dataclasses import dataclass, field
from typing import List
from enum import Enum

import pandas as pd
import numpy as np
import polars as pl


@dataclass
class Trade:
    """Represents a single executed trade."""
    timestamp: int
    side: str
    price: float
    quantity: int
    pnl: float = 0.0


@dataclass
class BacktestResult:
    """Aggregated results from a completed backtest run."""
    trades: List[Trade]
    total_pnl: float
    sharpe_ratio: float
    max_drawdown: float
    win_rate: float
    total_trades: int


class SimpleBacktester:
    """Event-driven backtester for NEXUS strategies using polars DataFrames.

    Converts a polars OHLCV DataFrame to pandas for signal computation, then
    simulates fills at bar-close prices with no slippage or commission.
    """

    def __init__(self, initial_capital: float = 1_000_000.0) -> None:
        self.capital = initial_capital
        self.trades: List[Trade] = []
        self.position: int = 0
        self.avg_cost: float = 0.0

    def run_ema_crossover(
        self,
        bars_df: pl.DataFrame,
        fast: int = 9,
        slow: int = 21,
    ) -> BacktestResult:
        """EMA crossover strategy backtest on OHLCV bars.

        Buys 100 units when the fast EMA crosses above the slow EMA and
        sells the entire position on the reverse crossover.

        Args:
            bars_df: Polars DataFrame with at least a 'close' column and
                     optionally a 'timestamp_ns' column.
            fast: Span for the fast EMA (default 9).
            slow: Span for the slow EMA (default 21).

        Returns:
            BacktestResult with trade list and aggregate statistics.
        """
        df = bars_df.to_pandas()
        df['ema_fast'] = df['close'].ewm(span=fast, adjust=False).mean()
        df['ema_slow'] = df['close'].ewm(span=slow, adjust=False).mean()
        df['signal'] = 0
        df.loc[df['ema_fast'] > df['ema_slow'], 'signal'] = 1
        df.loc[df['ema_fast'] < df['ema_slow'], 'signal'] = -1
        df['position_change'] = df['signal'].diff()

        trades: List[Trade] = []
        position: int = 0
        avg_cost: float = 0.0
        total_pnl: float = 0.0

        for _, row in df.iterrows():
            if row['position_change'] > 0 and position == 0:
                position = 100
                avg_cost = row['close']
                trades.append(Trade(int(row.get('timestamp_ns', 0)), 'BUY', row['close'], 100))
            elif row['position_change'] < 0 and position > 0:
                pnl = (row['close'] - avg_cost) * position
                trades.append(Trade(int(row.get('timestamp_ns', 0)), 'SELL', row['close'], position, pnl))
                total_pnl += pnl
                position = 0

        win_trades = [t for t in trades if t.pnl > 0]
        win_rate = len(win_trades) / len(trades) if trades else 0.0

        returns = df['close'].pct_change().dropna()
        sharpe = (returns.mean() / returns.std() * np.sqrt(252)) if returns.std() > 0 else 0.0

        cum = (1 + returns).cumprod()
        max_drawdown = float(((cum - cum.cummax()) / cum.cummax()).min()) if len(cum) > 0 else 0.0

        return BacktestResult(trades, total_pnl, sharpe, max_drawdown, win_rate, len(trades))

    def print_summary(self, result: BacktestResult) -> None:
        """Pretty-print a BacktestResult to stdout."""
        print("=== NEXUS Backtest Results ===")
        print(f"Total Trades:   {result.total_trades}")
        print(f"Total PnL:     ₹{result.total_pnl:,.2f}")
        print(f"Win Rate:       {result.win_rate:.1%}")
        print(f"Sharpe Ratio:   {result.sharpe_ratio:.3f}")
        print(f"Max Drawdown:   {result.max_drawdown:.2%}")

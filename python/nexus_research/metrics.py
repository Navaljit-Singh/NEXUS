"""Risk-adjusted performance metrics for NEXUS strategy evaluation."""

from typing import List

import numpy as np


def sharpe_ratio(
    returns: List[float],
    risk_free_rate: float = 0.065,
    periods_per_year: int = 252,
) -> float:
    """Compute the annualised Sharpe Ratio.

    Args:
        returns: Sequence of periodic returns (e.g. daily).
        risk_free_rate: Annual risk-free rate (default 6.5% for India).
        periods_per_year: Number of periods in a year (252 for daily).

    Returns:
        Annualised Sharpe Ratio, or 0.0 if standard deviation is negligible.
    """
    arr = np.array(returns)
    rf = risk_free_rate / periods_per_year
    excess = arr - rf
    return float((excess.mean() / excess.std()) * np.sqrt(periods_per_year)) if excess.std() > 1e-10 else 0.0


def max_drawdown(equity_curve: List[float]) -> float:
    """Compute the maximum drawdown from an equity curve.

    Args:
        equity_curve: Sequence of portfolio values over time.

    Returns:
        Maximum drawdown as a negative fraction (e.g. -0.25 for -25%).
    """
    arr = np.array(equity_curve)
    peak = np.maximum.accumulate(arr)
    dd = (arr - peak) / np.where(peak > 0, peak, 1)
    return float(dd.min())


def calmar_ratio(equity_curve: List[float], periods_per_year: int = 252) -> float:
    """Compute the Calmar Ratio (annualised return / max drawdown magnitude).

    Args:
        equity_curve: Sequence of portfolio values over time.
        periods_per_year: Number of periods in a year.

    Returns:
        Calmar Ratio, or 0.0 if max drawdown is negligible.
    """
    arr = np.array(equity_curve)
    returns = np.diff(arr) / arr[:-1]
    ann_return = (1 + returns.mean()) ** periods_per_year - 1
    mdd = abs(max_drawdown(equity_curve))
    return ann_return / mdd if mdd > 1e-10 else 0.0


def sortino_ratio(
    returns: List[float],
    risk_free_rate: float = 0.065,
    periods_per_year: int = 252,
) -> float:
    """Compute the annualised Sortino Ratio (penalises only downside volatility).

    Args:
        returns: Sequence of periodic returns.
        risk_free_rate: Annual risk-free rate (default 6.5% for India).
        periods_per_year: Number of periods in a year.

    Returns:
        Annualised Sortino Ratio.
    """
    arr = np.array(returns)
    rf = risk_free_rate / periods_per_year
    excess = arr - rf
    downside = excess[excess < 0]
    downside_std = np.sqrt((downside ** 2).mean()) if len(downside) > 0 else 1e-10
    return float((excess.mean() / downside_std) * np.sqrt(periods_per_year))

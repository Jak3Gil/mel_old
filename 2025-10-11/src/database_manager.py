#!/usr/bin/env python3
"""
Melvin Research Database Manager

Provides persistent, queryable storage for all diagnostic runs, metrics,
qualitative samples, and dataset metadata. Built on SQLite for longevity,
portability, and zero-configuration operation.

Design Principles:
- Longevity: SQLite works for decades, single portable file
- Reproducibility: Every entry tied to Git hash + tag
- Resilience: Complements CSV logs (redundancy)
- Query Power: SQL joins between metrics, samples, datasets
- Simplicity: No server, just one .db file

Usage:
    from database_manager import MelvinResearchDB
    
    db = MelvinResearchDB()
    run_id = db.log_run({
        'entry_number': 2,
        'date': '2025-10-11',
        'git_hash': 'abc123',
        ...
    })
    db.log_sample(run_id, 'fire→water', 'heat, steam, cool', 'weak', 'First link!')

Author: AI Research Infrastructure
Created: October 11, 2025
"""

import sqlite3
import os
import json
import datetime
from typing import Optional, Dict, List, Any, Tuple
from pathlib import Path


class MelvinResearchDB:
    """
    Manages the Melvin research database.
    
    Provides methods for:
    - Logging experiment runs with metrics
    - Logging qualitative samples
    - Logging dataset metadata
    - Querying historical data
    - Exporting results
    """
    
    def __init__(self, db_path: str = "melvin_research.db"):
        """
        Initialize database connection.
        
        Args:
            db_path: Path to SQLite database file (default: melvin_research.db)
        """
        self.db_path = db_path
        self.conn: Optional[sqlite3.Connection] = None
        self._connect()
        self._initialize_schema()
    
    def _connect(self) -> None:
        """Establish database connection."""
        try:
            self.conn = sqlite3.connect(self.db_path)
            self.conn.row_factory = sqlite3.Row  # Enable column access by name
            print(f"✅ Connected to database: {self.db_path}")
        except sqlite3.Error as e:
            print(f"❌ Database connection error: {e}")
            raise
    
    def _initialize_schema(self) -> None:
        """
        Create database tables if they don't exist.
        Idempotent - safe to call multiple times.
        """
        cursor = self.conn.cursor()
        
        # Table: runs - Main experiment tracking
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS runs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                entry_number INTEGER,
                date TEXT NOT NULL,
                timestamp TEXT NOT NULL,
                git_hash TEXT,
                tag TEXT,
                data_version TEXT,
                data_layer TEXT,
                data_size TEXT,
                similarity_before REAL,
                similarity_after REAL,
                entropy_before REAL,
                entropy_after REAL,
                leap_success_before REAL,
                leap_success_after REAL,
                lambda_graph_bias REAL,
                leap_entropy_threshold REAL,
                learning_rate_embeddings REAL,
                nodes_count INTEGER,
                edges_count INTEGER,
                notes TEXT,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
        """)
        
        # Table: samples - Qualitative outputs
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS samples (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                run_id INTEGER NOT NULL,
                prompt TEXT NOT NULL,
                output TEXT,
                quality TEXT,
                entropy REAL,
                similarity REAL,
                comment TEXT,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (run_id) REFERENCES runs(id) ON DELETE CASCADE
            )
        """)
        
        # Table: datasets - Data lineage tracking
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS datasets (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                version_path TEXT UNIQUE NOT NULL,
                description TEXT,
                size_lines INTEGER,
                size_bytes INTEGER,
                source TEXT,
                quality TEXT,
                md5_hash TEXT,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
        """)
        
        # Indexes for common queries
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_runs_date ON runs(date)
        """)
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_runs_entry ON runs(entry_number)
        """)
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_runs_similarity ON runs(similarity_after)
        """)
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_samples_run ON samples(run_id)
        """)
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_samples_quality ON samples(quality)
        """)
        
        self.conn.commit()
        print("✅ Database schema initialized")
    
    def log_run(self, run_data: Dict[str, Any]) -> int:
        """
        Log a complete diagnostic run.
        
        Args:
            run_data: Dictionary with keys:
                - entry_number (int)
                - date (str, YYYY-MM-DD)
                - git_hash (str)
                - tag (str, optional)
                - data_version (str)
                - data_layer (str)
                - data_size (str)
                - similarity_before (float)
                - similarity_after (float)
                - entropy_before (float)
                - entropy_after (float)
                - leap_success_before (float)
                - leap_success_after (float)
                - lambda_graph_bias (float)
                - leap_entropy_threshold (float)
                - learning_rate_embeddings (float)
                - notes (str, optional)
        
        Returns:
            run_id: The ID of the inserted run
        """
        cursor = self.conn.cursor()
        
        # Add timestamp if not provided
        if 'timestamp' not in run_data:
            run_data['timestamp'] = datetime.datetime.utcnow().isoformat()
        
        try:
            cursor.execute("""
                INSERT INTO runs (
                    entry_number, date, timestamp, git_hash, tag, data_version,
                    data_layer, data_size, similarity_before, similarity_after,
                    entropy_before, entropy_after, leap_success_before, 
                    leap_success_after, lambda_graph_bias, leap_entropy_threshold,
                    learning_rate_embeddings, nodes_count, edges_count, notes
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                run_data.get('entry_number'),
                run_data.get('date'),
                run_data.get('timestamp'),
                run_data.get('git_hash'),
                run_data.get('tag'),
                run_data.get('data_version'),
                run_data.get('data_layer'),
                run_data.get('data_size'),
                run_data.get('similarity_before'),
                run_data.get('similarity_after'),
                run_data.get('entropy_before'),
                run_data.get('entropy_after'),
                run_data.get('leap_success_before'),
                run_data.get('leap_success_after'),
                run_data.get('lambda_graph_bias'),
                run_data.get('leap_entropy_threshold'),
                run_data.get('learning_rate_embeddings'),
                run_data.get('nodes_count'),
                run_data.get('edges_count'),
                run_data.get('notes')
            ))
            self.conn.commit()
            run_id = cursor.lastrowid
            print(f"✅ Logged run {run_id} (Entry {run_data.get('entry_number')}, {run_data.get('date')})")
            return run_id
        except sqlite3.Error as e:
            print(f"❌ Error logging run: {e}")
            self.conn.rollback()
            raise
    
    def log_sample(
        self, 
        run_id: int, 
        prompt: str, 
        output: str, 
        quality: str,
        entropy: Optional[float] = None,
        similarity: Optional[float] = None,
        comment: Optional[str] = None
    ) -> int:
        """
        Log a qualitative sample from a run.
        
        Args:
            run_id: ID of the associated run
            prompt: Test prompt (e.g., "fire→water")
            output: Generated predictions (e.g., "heat, steam, cool")
            quality: Link quality (strong, moderate, weak, none)
            entropy: Entropy reduction value
            similarity: Context similarity value
            comment: Optional observation note
        
        Returns:
            sample_id: The ID of the inserted sample
        """
        cursor = self.conn.cursor()
        
        try:
            cursor.execute("""
                INSERT INTO samples (
                    run_id, prompt, output, quality, entropy, similarity, comment
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (run_id, prompt, output, quality, entropy, similarity, comment))
            self.conn.commit()
            sample_id = cursor.lastrowid
            print(f"✅ Logged sample {sample_id} for run {run_id}")
            return sample_id
        except sqlite3.Error as e:
            print(f"❌ Error logging sample: {e}")
            self.conn.rollback()
            raise
    
    def log_dataset(self, dataset_data: Dict[str, Any]) -> int:
        """
        Log dataset metadata.
        
        Args:
            dataset_data: Dictionary with keys:
                - version_path (str): Path to versioned_data/TIMESTAMP/
                - description (str): Human-readable description
                - size_lines (int): Number of lines
                - size_bytes (int): File size in bytes
                - source (str): Data source (e.g., "TinyChat")
                - quality (str): Quality rating (high, medium, low)
                - md5_hash (str, optional): File checksum
        
        Returns:
            dataset_id: The ID of the inserted dataset
        """
        cursor = self.conn.cursor()
        
        try:
            cursor.execute("""
                INSERT OR REPLACE INTO datasets (
                    version_path, description, size_lines, size_bytes,
                    source, quality, md5_hash
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (
                dataset_data.get('version_path'),
                dataset_data.get('description'),
                dataset_data.get('size_lines'),
                dataset_data.get('size_bytes'),
                dataset_data.get('source'),
                dataset_data.get('quality'),
                dataset_data.get('md5_hash')
            ))
            self.conn.commit()
            dataset_id = cursor.lastrowid
            print(f"✅ Logged dataset: {dataset_data.get('version_path')}")
            return dataset_id
        except sqlite3.Error as e:
            print(f"❌ Error logging dataset: {e}")
            self.conn.rollback()
            raise
    
    def fetch_runs(
        self, 
        where_clause: Optional[str] = None,
        order_by: str = "date DESC",
        limit: Optional[int] = None
    ) -> List[Dict[str, Any]]:
        """
        Query runs with optional filtering.
        
        Args:
            where_clause: SQL WHERE clause (e.g., "similarity_after >= 0.5")
            order_by: ORDER BY clause (default: date DESC)
            limit: Maximum number of results
        
        Returns:
            List of run dictionaries
        
        Examples:
            # All runs above 0.5 similarity
            fetch_runs("similarity_after >= 0.5")
            
            # Last 10 runs
            fetch_runs(limit=10)
            
            # Runs from Entry 5 onwards
            fetch_runs("entry_number >= 5", order_by="entry_number ASC")
        """
        cursor = self.conn.cursor()
        
        query = "SELECT * FROM runs"
        if where_clause:
            query += f" WHERE {where_clause}"
        query += f" ORDER BY {order_by}"
        if limit:
            query += f" LIMIT {limit}"
        
        try:
            cursor.execute(query)
            rows = cursor.fetchall()
            return [dict(row) for row in rows]
        except sqlite3.Error as e:
            print(f"❌ Error fetching runs: {e}")
            return []
    
    def fetch_samples(
        self, 
        run_id: Optional[int] = None,
        quality: Optional[str] = None,
        limit: Optional[int] = None
    ) -> List[Dict[str, Any]]:
        """
        Query qualitative samples.
        
        Args:
            run_id: Filter by specific run (optional)
            quality: Filter by quality (strong, moderate, weak, none)
            limit: Maximum number of results
        
        Returns:
            List of sample dictionaries
        
        Examples:
            # All samples from Entry 7
            fetch_samples(run_id=7)
            
            # All strong-quality samples
            fetch_samples(quality='strong')
        """
        cursor = self.conn.cursor()
        
        query = "SELECT * FROM samples"
        conditions = []
        params = []
        
        if run_id is not None:
            conditions.append("run_id = ?")
            params.append(run_id)
        if quality:
            conditions.append("quality = ?")
            params.append(quality)
        
        if conditions:
            query += " WHERE " + " AND ".join(conditions)
        query += " ORDER BY created_at DESC"
        if limit:
            query += f" LIMIT {limit}"
        
        try:
            cursor.execute(query, params)
            rows = cursor.fetchall()
            return [dict(row) for row in rows]
        except sqlite3.Error as e:
            print(f"❌ Error fetching samples: {e}")
            return []
    
    def fetch_run_with_samples(self, run_id: int) -> Optional[Dict[str, Any]]:
        """
        Fetch a complete run with all its samples.
        
        Args:
            run_id: ID of the run
        
        Returns:
            Dictionary with 'run' and 'samples' keys, or None if not found
        """
        runs = self.fetch_runs(f"id = {run_id}")
        if not runs:
            return None
        
        run = runs[0]
        samples = self.fetch_samples(run_id=run_id)
        
        return {
            'run': run,
            'samples': samples
        }
    
    def get_stats(self) -> Dict[str, Any]:
        """
        Get database statistics.
        
        Returns:
            Dictionary with counts and metrics
        """
        cursor = self.conn.cursor()
        
        stats = {}
        
        # Total runs
        cursor.execute("SELECT COUNT(*) FROM runs")
        stats['total_runs'] = cursor.fetchone()[0]
        
        # Total samples
        cursor.execute("SELECT COUNT(*) FROM samples")
        stats['total_samples'] = cursor.fetchone()[0]
        
        # Total datasets
        cursor.execute("SELECT COUNT(*) FROM datasets")
        stats['total_datasets'] = cursor.fetchone()[0]
        
        # Latest run
        cursor.execute("SELECT date, similarity_after FROM runs ORDER BY date DESC LIMIT 1")
        latest = cursor.fetchone()
        if latest:
            stats['latest_run_date'] = latest[0]
            stats['latest_similarity'] = latest[1]
        
        # Best similarity
        cursor.execute("SELECT MAX(similarity_after) FROM runs")
        stats['best_similarity'] = cursor.fetchone()[0] or 0.0
        
        # Strong samples
        cursor.execute("SELECT COUNT(*) FROM samples WHERE quality = 'strong'")
        stats['strong_samples'] = cursor.fetchone()[0]
        
        return stats
    
    def export_to_json(self, output_path: str) -> None:
        """
        Export entire database to JSON (for backup/sharing).
        
        Args:
            output_path: Path to output JSON file
        """
        data = {
            'runs': self.fetch_runs(),
            'samples': self.fetch_samples(),
            'datasets': self._fetch_all_datasets(),
            'exported_at': datetime.datetime.utcnow().isoformat()
        }
        
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"✅ Exported database to {output_path}")
    
    def _fetch_all_datasets(self) -> List[Dict[str, Any]]:
        """Fetch all datasets."""
        cursor = self.conn.cursor()
        cursor.execute("SELECT * FROM datasets ORDER BY created_at DESC")
        rows = cursor.fetchall()
        return [dict(row) for row in rows]
    
    def close(self) -> None:
        """Close database connection."""
        if self.conn:
            self.conn.close()
            print("✅ Database connection closed")
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - ensures connection is closed."""
        self.close()


# Command-line interface
if __name__ == "__main__":
    import sys
    import argparse
    
    parser = argparse.ArgumentParser(description="Melvin Research Database Manager")
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    
    # Init command
    parser_init = subparsers.add_parser('init', help='Initialize database')
    
    # Log run command
    parser_log_run = subparsers.add_parser('log_run', help='Log a diagnostic run')
    parser_log_run.add_argument('--entry', type=int, required=True)
    parser_log_run.add_argument('--date', required=True)
    parser_log_run.add_argument('--git-hash', required=True)
    parser_log_run.add_argument('--tag', default='')
    parser_log_run.add_argument('--data-version', required=True)
    parser_log_run.add_argument('--data-layer', required=True)
    parser_log_run.add_argument('--data-size', required=True)
    parser_log_run.add_argument('--sim-before', type=float, required=True)
    parser_log_run.add_argument('--sim-after', type=float, required=True)
    parser_log_run.add_argument('--ent-before', type=float, required=True)
    parser_log_run.add_argument('--ent-after', type=float, required=True)
    parser_log_run.add_argument('--success-before', type=float, required=True)
    parser_log_run.add_argument('--success-after', type=float, required=True)
    parser_log_run.add_argument('--lambda', type=float, dest='lambda_val', required=True)
    parser_log_run.add_argument('--threshold', type=float, required=True)
    parser_log_run.add_argument('--lr', type=float, required=True)
    parser_log_run.add_argument('--nodes', type=int, default=0, dest='nodes_count')
    parser_log_run.add_argument('--edges', type=int, default=0, dest='edges_count')
    parser_log_run.add_argument('--notes', default='')
    
    # Stats command
    parser_stats = subparsers.add_parser('stats', help='Show database statistics')
    
    # Query command
    parser_query = subparsers.add_parser('query', help='Query runs')
    parser_query.add_argument('where', nargs='?', help='WHERE clause')
    parser_query.add_argument('--limit', type=int)
    
    # Export command
    parser_export = subparsers.add_parser('export', help='Export to JSON')
    parser_export.add_argument('output', help='Output JSON file')
    
    args = parser.parse_args()
    
    if args.command == 'init':
        db = MelvinResearchDB()
        print("✅ Database initialized successfully")
        db.close()
    
    elif args.command == 'log_run':
        db = MelvinResearchDB()
        run_id = db.log_run({
            'entry_number': args.entry,
            'date': args.date,
            'git_hash': args.git_hash,
            'tag': args.tag,
            'data_version': args.data_version,
            'data_layer': args.data_layer,
            'data_size': args.data_size,
            'similarity_before': args.sim_before,
            'similarity_after': args.sim_after,
            'entropy_before': args.ent_before,
            'entropy_after': args.ent_after,
            'leap_success_before': args.success_before,
            'leap_success_after': args.success_after,
            'lambda_graph_bias': args.lambda_val,
            'leap_entropy_threshold': args.threshold,
            'learning_rate_embeddings': args.lr,
            'nodes_count': args.nodes_count,
            'edges_count': args.edges_count,
            'notes': args.notes
        })
        print(f"✅ Created run with ID: {run_id}")
        db.close()
    
    elif args.command == 'stats':
        db = MelvinResearchDB()
        stats = db.get_stats()
        print("\n📊 Database Statistics")
        print("=" * 50)
        for key, value in stats.items():
            print(f"  {key:.<40} {value}")
        print("=" * 50)
        db.close()
    
    elif args.command == 'query':
        db = MelvinResearchDB()
        runs = db.fetch_runs(where_clause=args.where, limit=args.limit)
        print(f"\n📊 Found {len(runs)} runs")
        print("=" * 80)
        for run in runs:
            print(f"Run {run['id']:>3} | Entry {run['entry_number']:>2} | {run['date']} | Sim: {run['similarity_after']:.3f}")
        db.close()
    
    elif args.command == 'export':
        db = MelvinResearchDB()
        db.export_to_json(args.output)
        db.close()
    
    else:
        parser.print_help()


# exchange_server.py

import argparse
import socket
import json
import time
import threading
import pandas as pd
import os
from glob import glob
from tqdm import tqdm

def load_all_messages(data_dir, symbols):
    message_data = []
    for file in glob(os.path.join(data_dir, '*_message_*.csv')):
        symbol = os.path.basename(file).split('_')[0]
        if symbol not in symbols:
            continue
        df = pd.read_csv(file, header=None)
        df.columns = ["time", "type", "order_id", "size", "price", "direction"]
        df['symbol'] = symbol
        df['price'] = df['price'] / 10000
        message_data.append(df)
    full_df = pd.concat(message_data)
    full_df.sort_values(by="time", inplace=True)
    return full_df

def stream_messages(conn, addr, message_df, delay_per_event=0.01):
    print(f"Client connected: {addr}")
    sending_df = message_df[:50]
    with conn:
        for _, row in tqdm(sending_df.iterrows(), total=len(sending_df), desc="Sending events"):
            event = {
                "timestamp": row["time"],
                "symbol": row["symbol"],
                "price": round(row["price"], 4),
                "type": int(row["type"]),
                "order_id": int(row["order_id"]),
                "direction": int(row["direction"]),
                "size": int(row["size"])
            }
            conn.sendall((json.dumps(event) + "\n").encode('utf-8'))
            time.sleep(delay_per_event)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', type=int, required=True, help='Port to serve feed on')
    parser.add_argument('--symbols', nargs='+', required=True, help='Symbols to stream')
    parser.add_argument('--data-dir', default='./data', help='Path to message CSVs')
    args = parser.parse_args()

    print(f"Loading data for symbols: {args.symbols}")
    message_df = load_all_messages(args.data_dir, set(args.symbols))
    print(f"Loaded {len(message_df)} messages for {args.symbols}")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('127.0.0.1', args.port))
        s.listen(5)
        print(f"🟢 Feed server for {args.symbols} started on port {args.port}...")

        while True:
            conn, addr = s.accept()
            thread = threading.Thread(target=stream_messages, args=(conn, addr, message_df))
            thread.start()

if __name__ == "__main__":
    main()




# import socket
# import json
# import time
# import threading
# import pandas as pd
# import os
# from glob import glob
# from tqdm import tqdm

# DATA_DIR = './data'  # Folder where your message CSVs are stored

# def load_all_messages(data_dir):
#     message_data = []
#     for file in glob(os.path.join(data_dir, '*_message_*.csv')):
#         symbol = os.path.basename(file).split('_')[0]
#         df = pd.read_csv(file, header=None)
#         df.columns = ["time", "type", "order_id", "size", "price", "direction"]
#         df['symbol'] = symbol
#         df['price'] = df['price'] / 10000  # convert price
#         message_data.append(df)
#     full_df = pd.concat(message_data)
#     full_df.sort_values(by="time", inplace=True)
#     # full_df.to_csv("Full_data.csv")
#     return full_df

# def stream_messages(conn, addr, message_df, delay_per_event=0.01):
#     print(f"Client connected: {addr}")
#     sending_df = message_df[:5000]
#     with conn:
#         for _, row in tqdm(sending_df.iterrows(), total=len(sending_df), desc="Sending events"):
#             event = {
#                 "timestamp": row["time"],
#                 "symbol": row["symbol"],
#                 "price": round(row["price"], 4) / 10000,
#                 "type": int(row["type"]),
#                 "order_id": int(row["order_id"]),
#                 "direction": int(row["direction"]),
#                 "size": int(row["size"])
#             }
#             conn.sendall((json.dumps(event) + "\n").encode('utf-8'))
#             time.sleep(delay_per_event)


# def main():
#     host = '127.0.0.1'
#     port = 5000
#     print("Loading all message CSVs...")
#     message_df = load_all_messages(DATA_DIR)
#     # message_df = pd.read_csv(DATA_DIR + '/sample.csv')
#     print(f"Loaded {len(message_df)} total events from {DATA_DIR}")

#     with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
#         s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#         s.bind((host, port))
#         s.listen(5)
#         print(f"Streaming server started on {host}:{port}...")

#         while True:
#             conn, addr = s.accept()
#             thread = threading.Thread(target=stream_messages, args=(conn, addr, message_df))
#             thread.start()

# if __name__ == "__main__":
#     main()


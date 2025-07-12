import socket
import json
import time
import threading
import pandas as pd
import os
from glob import glob

DATA_DIR = './data'  # Folder where your message CSVs are stored

def load_all_messages(data_dir):
    message_data = []
    for file in glob(os.path.join(data_dir, '*_message_*.csv')):
        symbol = os.path.basename(file).split('_')[0]
        df = pd.read_csv(file, header=None)
        df.columns = ["time", "type", "order_id", "size", "price", "direction"]
        df['symbol'] = symbol
        df['price'] = df['price'] / 10000  # convert price
        message_data.append(df)
    full_df = pd.concat(message_data)
    full_df.sort_values(by="time", inplace=True)
    # full_df.to_csv("Full_data.csv")
    return full_df

def stream_messages(conn, addr, message_df, delay_per_event=0.01):
    print(f"Client connected: {addr}")
    with conn:
        for _, row in message_df[:100].iterrows():
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
    host = '127.0.0.1'
    port = 5000
    print("Loading all message CSVs...")
    message_df = load_all_messages(DATA_DIR)
    print(f"Loaded {len(message_df)} total events from {DATA_DIR}")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen(5)
        print(f"Streaming server started on {host}:{port}...")

        while True:
            conn, addr = s.accept()
            thread = threading.Thread(target=stream_messages, args=(conn, addr, message_df))
            thread.start()

if __name__ == "__main__":
    main()



# import socket
# import json
# import time
# import random
# import string
# import threading

# def generate_random_tickers(n, length=4):
#     tickers = set()
#     while len(tickers) < n:
#         ticker = ''.join(random.choices(string.ascii_uppercase, k=length))
#         tickers.add(ticker)
#     return list(tickers)

# def generate_trade(timestamp, symbols):
#     symbol = random.choice(symbols)
#     price = round(random.uniform(100, 500), 2)
#     volume = random.randint(10, 1000)
#     return {
#         "symbol": symbol,
#         "price": price,
#         "volume": volume,
#         "timestamp": timestamp
#     }

# def handle_client(conn, addr, base_timestamp, n_trades, interval_us, symbols):
#     print(f"Connection from {addr}")
#     with conn:
#         for i in range(n_trades):
#             timestamp = base_timestamp + i * interval_us
#             trade = generate_trade(timestamp, symbols)
#             message = json.dumps(trade) + "\n"
#             conn.sendall(message.encode('utf-8'))
#             time.sleep(0.01)  # simulate ~10 trades/sec

# def main():
#     host = '127.0.0.1'
#     port = 5000
#     N_TRADES = 1000
#     MICROSECOND_INTERVAL = 100_000  # 100 ms
#     base_timestamp = 1_750_000_000_000_000

#     N_TICKERS = 4  # You can change this to any number
#     tickers = generate_random_tickers(N_TICKERS)

#     with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
#         s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#         s.bind((host, port))
#         s.listen(5)
#         print(f"Listening on {host}:{port} with {N_TICKERS} tickers...")

#         while True:
#             conn, addr = s.accept()
#             thread = threading.Thread(
#                 target=handle_client,
#                 args=(conn, addr, base_timestamp, N_TRADES, MICROSECOND_INTERVAL, tickers)
#             )
#             thread.start()

# if __name__ == "__main__":
#     main()

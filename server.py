import socket
import json
import time
import random

def generate_trade(timestamp):
    symbols = ["AAPL", "GOOG", "TSLA", "MSFT"]
    symbol = random.choice(symbols)
    price = round(random.uniform(100, 500), 2)
    volume = random.randint(10, 1000)
    return {
        "symbol": symbol,
        "price": price,
        "volume": volume,
        "timestamp": timestamp
    }

def main():
    host = '127.0.0.1'
    port = 5000
    N_TRADES = 500
    MICROSECOND_INTERVAL = 100_000  # 100 ms
    base_timestamp = 1_750_000_000_000_000  # any large constant for realism

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen(1)
        print(f"Listening on {host}:{port}...")

        conn, addr = s.accept()
        with conn:
            print(f"Connection from {addr}")
            for i in range(N_TRADES):
                timestamp = base_timestamp + i * MICROSECOND_INTERVAL
                trade = generate_trade(timestamp)
                message = json.dumps(trade) + "\n"
                conn.sendall(message.encode('utf-8'))
                time.sleep(0.01)  # simulate ~10 trades/sec

if __name__ == "__main__":
    main()

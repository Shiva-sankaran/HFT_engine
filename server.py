import socket
import json
import time
import random
import string
import threading

def generate_random_tickers(n, length=4):
    tickers = set()
    while len(tickers) < n:
        ticker = ''.join(random.choices(string.ascii_uppercase, k=length))
        tickers.add(ticker)
    return list(tickers)

def generate_trade(timestamp, symbols):
    symbol = random.choice(symbols)
    price = round(random.uniform(100, 500), 2)
    volume = random.randint(10, 1000)
    return {
        "symbol": symbol,
        "price": price,
        "volume": volume,
        "timestamp": timestamp
    }

def handle_client(conn, addr, base_timestamp, n_trades, interval_us, symbols):
    print(f"Connection from {addr}")
    with conn:
        for i in range(n_trades):
            timestamp = base_timestamp + i * interval_us
            trade = generate_trade(timestamp, symbols)
            message = json.dumps(trade) + "\n"
            conn.sendall(message.encode('utf-8'))
            time.sleep(0.01)  # simulate ~10 trades/sec

def main():
    host = '127.0.0.1'
    port = 5000
    N_TRADES = 100
    MICROSECOND_INTERVAL = 100_000  # 100 ms
    base_timestamp = 1_750_000_000_000_000

    N_TICKERS = 4  # You can change this to any number
    tickers = generate_random_tickers(N_TICKERS)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen(5)
        print(f"Listening on {host}:{port} with {N_TICKERS} tickers...")

        while True:
            conn, addr = s.accept()
            thread = threading.Thread(
                target=handle_client,
                args=(conn, addr, base_timestamp, N_TRADES, MICROSECOND_INTERVAL, tickers)
            )
            thread.start()

if __name__ == "__main__":
    main()

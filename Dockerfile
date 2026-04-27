FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential make ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make clean && make release && make gui

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libc6 bash \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/progvoraz /app/progvoraz
COPY --from=builder /app/progvoraz_gui /app/progvoraz_gui
COPY --from=builder /app/monedas.txt /app/monedas.txt
COPY --from=builder /app/stock.txt /app/stock.txt
COPY --from=builder /app/docker /app/docker

RUN chmod +x /app/docker/entrypoint.sh /app/docker/run_all_modes_tests.sh

ENTRYPOINT ["/app/docker/entrypoint.sh"]
CMD ["console"]

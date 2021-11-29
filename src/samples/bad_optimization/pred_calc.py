def calc(less_than_zero):
    num_cycles = 300 if less_than_zero < 0 else 400
    addition = 3.0 if less_than_zero < 0 else 64.0
    subtraction = 0.0 if less_than_zero < 0 else 0.3
    power = 7000 if less_than_zero < 0 else 4000

    res = 0.0
    for i in range(num_cycles):
        res += (addition + i)**(-power)
    return res

print(f"less than zero: f{calc(True)}")
print(f"more or equal than zero: f{calc(False)}")
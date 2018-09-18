from stdb import *
from pyalphatree import *

hold_days = 2
target_returns = 0.006
valid_sign_name = "valid_sign"
valid_sign_line = '(volume > 0)'

feature_list = [
    "(ts_rank(returns, 12.06470013) / (ma(high, 20) - ma(close, 20)))",
    "(ts_rank(close, 5) - sign(wma(delta(correlation(close, close, 5.11456013), 50), 11.82590008)))",
    "((close / (open + ts_min(open, 3))) - wma(delay(stddev(ma(returns, 18), 9), 2), 20.04509926))",
    "((close - high) - delta(ma(stddev(close, 10), 19), 7))",
    "(correlation((open / 2), open, 20) - (ts_min(ma((close - high), 19), 2.14593005) / ts_min(close, 11.57830048)))"]

if __name__ == '__main__':
    download_industry(['sz399005'], 'sz399005', 'data')
    cache_base('data')
    with AlphaForest() as af:
        af.load_db('data')
        codes = af.get_stock_codes()
        codes.sort()
        if len(codes) == 0:
            codes = af.get_market_codes()
            af.cache_codes_sign(valid_sign_name, valid_sign_line, codes)
            af.load_feature('miss')
            af.load_feature('date')
            af.load_feature("returns")
            af.load_sign(valid_sign_name)
            bi = AlphaBI(valid_sign_name, 0, 250, 6, 0.32, target_returns, 'rand', "returns")
            for i in range(len(feature_list)):
                print(feature_list[i])
                print("  discrimination:%.4f"%(bi.get_discrimination(feature_list[i])))
                print("  correlation:")
                for j in range(i+1, len(feature_list)):
                    corr = bi.get_correlation(feature_list[i], feature_list[j])
                    print("      %.4f:%s"%(corr, feature_list[j]))
            del bi

from stdb import *
from pyalphatree import *

hold_days = 2
target_returns = 0.006
valid_sign_name = "valid_sign"
valid_sign_line = '(volume > 0)'

feature_list = [
    "correlation(ts_rank(stddev((sum((close + close), 39) / 2), 20), 8), (ts_rank(close, 7) - ts_rank(sum(volume, 13), 3)), 5)",
    "((close / high) - stddev(ma(returns, 7), 3))",
    "correlation((lerp(open, open, 0.49) - delay(close, 20)), stddev(close, 5), 9)",
    "(ts_rank(open, 4) - correlation(ts_min(sum(low, 20), 5), ts_min(sum(high, 20), 3), 6))",
    "correlation(wma(ts_rank(stddev(volume, 20), 5), 7), ts_rank(volume, 4), 7)"]

if __name__ == '__main__':
    download_industry(['1399005'], '1399005', 'data')
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
            bi = AlphaBI(valid_sign_name, 0, 128, 16, 0.6, 'rand', "returns")
            for i in range(len(feature_list)):
                print(feature_list[i])
                print("  discrimination:%.4f"%(bi.get_discrimination(feature_list[i], target_returns)))
                print("  correlation:")
                for j in range(i+1, len(feature_list)):
                    corr = bi.get_correlation(feature_list[i], feature_list[j])
                    print("      %.4f:%s"%(corr, feature_list[j]))
            del bi

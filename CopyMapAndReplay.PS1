# copy maps and replays to local path for the use of local StarCraftII
# maps
Copy-Item -Filter *.SC2Map -Path '\\172.24.124.1\liuyongfeng_home\s2client-api-liu\s2client-api\maps\*'  -Destination 'H:\StarCraft II\Maps' -Force -Recurse
# replays
Copy-Item -Filter *.SC2Replay -Path '\\172.24.124.1\liuyongfeng_home\s2client-api-liu\s2client-api\replays\*' -Destination 'E:\MyDocument\StarCraft II\Accounts\238372333\5-S2-1-6115474\Replays\Multiplayer\Research' -Force -Recurse
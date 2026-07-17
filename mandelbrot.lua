local s_pack   = string.pack
local m_sqrt   = math.sqrt
local m_atan   = math.atan
local m_cos    = math.cos
local m_sin    = math.sin
local t_concat = table.concat

--chamada pelo main.c a cada novo frame, com os parametros atuais da
--camera (posicao, zoom, potencia) e o numero maximo de iteracoes
function calcular_frame(width, height, offset_x, offset_y, zoom, power, max_iter)
    local step_x = 3.0 / (zoom * width)
    local step_y = 3.0 / (zoom * height)
    local start_x = offset_x - (1.5 / zoom)
    local start_y = offset_y - (1.5 / zoom)

    --buffer com o resultado de cada pixel (3 doubles: iteracoes, parte real e
    --parte imaginaria do ponto de escape), concatenado no final em uma unica
    --string binaria e devolvido ao C
    local buffer = {}
    local b_idx = 1

    for y = 0, height - 1 do
        local ci = start_y + (y * step_y)

        for x = 0, width - 1 do
            local cr = start_x + (x * step_x)
            local zr, zi = 0.0, 0.0
            local n = 0

            if power == 2.0 then
                local zr2, zi2 = 0.0, 0.0
                while (zr2 + zi2 < 4.0) and (n < max_iter) do
                    zi = 2.0 * zr * zi + ci
                    zr = zr2 - zi2 + cr
                    zr2 = zr * zr
                    zi2 = zi * zi
                    n = n + 1
                end
            else
                while (zr * zr + zi * zi < 4.0) and (n < max_iter) do
                    local r = m_sqrt(zr * zr + zi * zi)
                    local theta = m_atan(zi, zr)

                    local r_pow = r ^ power
                    zr = r_pow * m_cos(power * theta) + cr
                    zi = r_pow * m_sin(power * theta) + ci
                    n = n + 1
                end
            end

            buffer[b_idx] = s_pack("ddd", n, zr, zi)
            b_idx = b_idx + 1
        end
    end

    --junta todos os pixels em uma unica string binaria na memoria e devolve
    --para o C como retorno da funcao
    return t_concat(buffer)
end

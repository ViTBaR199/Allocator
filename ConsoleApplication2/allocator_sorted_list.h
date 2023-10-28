#ifndef DATA_STRUCTURES_CPP_MEMORY_WITH_SORTED_LIST_DEALLOCATION_H
#define DATA_STRUCTURES_CPP_MEMORY_WITH_SORTED_LIST_DEALLOCATION_H

#include "typename_holder.h"
#include "logger.h"
#include "logger_holder.h"
#include "allocator.h"
#include "allocator_fit_allocation.h"
#include "allocator_holder.h"

class allocator_sorted_list final:
    public allocator_fit_allocation,
    protected logger_holder,
    protected typename_holder,
    protected allocator_holder
{

private:

    void *_trusted_memory;

public:
    //Он выделяет память и устанавливает начальные параметры, такие как размер памяти, 
    //указатель на внешний аллокатор (если есть), режим аллокации и другие.
    explicit allocator_sorted_list(
        size_t memory_size,
        allocator *outer_allocator = nullptr,
        logger *logger = nullptr,
        allocator_fit_allocation::allocation_mode allocation_mode = allocator_fit_allocation::allocation_mode::first_fit);

    allocator_sorted_list(
        allocator_sorted_list const &other) = delete;

    allocator_sorted_list& operator=(
        allocator_sorted_list const &other) = delete;

    //Деструктор класса. Этот метод освобождает выделенную память при уничтожении объекта 
    ~allocator_sorted_list() noexcept;

private:

    //Возвращает размер выделенной памяти в объекте
    [[nodiscard]] size_t get_trusted_memory_size() const noexcept override;

    //Возвращает режим аллокации, установленный для объекта
    [[nodiscard]] allocator_fit_allocation::allocation_mode get_allocation_mode() const noexcept override;

    //Возвращает размер блока служебной информации аллокатора.
    [[nodiscard]] size_t get_allocator_service_block_size() const noexcept override;

    //Возвращает размер блока служебной информации доступных блоков памяти.
    [[nodiscard]] size_t get_available_block_service_block_size() const noexcept override;

    //Возвращает размер блока служебной информации занятых блоков памяти.
    [[nodiscard]] size_t get_occupied_block_service_block_size() const noexcept override;

    //Возвращает адрес указателя на первый доступный блок памяти.
    [[nodiscard]] void **get_first_available_block_address_address() const noexcept override;

    //Возвращает адрес первого доступного блока памяти.
    [[nodiscard]] void *get_first_available_block_address() const noexcept override;

    //Возвращает размер доступного блока по адресу current_block_address.
    size_t get_available_block_size(
        void const *current_block_address) const override;

    //Возвращает адрес следующего доступного блока после current_block_address.
    void * get_available_block_next_available_block_address(
        void const *current_block_address) const override;

    //Возвращает размер занятого блока по адресу current_block_address.
    size_t get_occupied_block_size(
        void const *current_block_address) const override;

    //Выводит состояние блоков памяти в объекте allocator_sorted_list (занятые и доступные блоки).
    void dump_trusted_memory_blocks_state() const override;

public:

    //Выделяет блок памяти заданного размера requested_block_size и возвращает указатель на начало выделенного блока.
    void *allocate(
        size_t requested_block_size) override;

    //Освобождает блок памяти по указанному адресу block_to_deallocate_address.
    void deallocate(
        void *block_to_deallocate_address) override;

    //Перераспределяет блок памяти, изменяя его размер на new_block_size. Если возможно, это делается без перемещения данных.
    [[nodiscard]] void *reallocate(
        void *block_to_reallocate_address,
        size_t new_block_size) override;

    //Перераспределяет блок памяти через указатель на указатель. Если перераспределение прошло успешно, указатель на блок обновляется.
    bool reallocate(
        void **block_to_reallocate_address_address,
        size_t new_block_size) override;

public:

    //Устанавливает режим аллокации для объекта 
    void setup_allocation_mode(
        allocator_fit_allocation::allocation_mode mode) override;

private:
    
    //Возвращает указатель на объект logger, связанный с данным аллокатором.
    [[nodiscard]] logger *get_logger() const noexcept override;

private:

    //Возвращает строку с именем класса (allocator_sorted_list).
    [[nodiscard]] std::string get_typename() const noexcept override;

private:

    //Возвращает указатель на внешний аллокатор (если он есть).
    [[nodiscard]] allocator *get_allocator() const noexcept override;

};

#endif //DATA_STRUCTURES_CPP_MEMORY_WITH_SORTED_LIST_DEALLOCATION_H
